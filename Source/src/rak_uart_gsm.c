/*
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 */
#include "rak_uart_gsm.h"
#include "gps.h"
#include "mqttnet.h"

#define  GSM_RXBUF_MAXSIZE           1600

static uint16_t rxReadIndex = 0;
static uint16_t rxWriteIndex = 0;
static uint16_t rxCount = 0;
static uint8_t Gsm_RxBuf[GSM_RXBUF_MAXSIZE];

//extern uint8_t uart2_rx_data;
extern int R485_UART_TxBuf(uint8_t *buffer, int nbytes);
extern GSM_RECIEVE_TYPE g_type;

char GSM_CMD[60] = { 0 };
uint8_t GSM_RSP[1600] = { 0 };
char resp[1500] = { 0 };
extern char CMD[128];
extern char RSP[128];
char gps_data[512] = { 0 };
extern tNmeaGpsData NmeaGpsData;
extern char post_data[1024];
#define  GPS_FORMAT                             \
	"{\r\n"                                                 \
	"\"NmeaDataType\" : %s,\r\n"                            \
	"\"NmeaUtcTime\" : %s,\r\n"                             \
	"\"NmeaLatitude\" : %s,\r\n"                            \
	"\"NmeaLatitudePole\" : %s,\r\n"                                \
	"\"NmeaLongitude\" : %s,\r\n"                           \
	"\"NmeaLongitudePole\" : %s,\r\n"                               \
	"\"NmeaAltitude\" : %s,\r\n" \
	"\"NmeaAltitudeUnit\" : %s,\r\n" \
	"\"NmeaHeightGeoid\" : %s,\r\n"         \
	"\"NmeaHeightGeoidUnit\" : %s,\r\n"                     \
	"}\r\n"
/*
 *********************************************************************************************************
 *                                         FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

void  AppTaskGPRS(void const *argument);

void delay_ms(uint32_t ms)
{
	nrf_delay_ms(ms);
}

int GSM_UART_TxBuf(uint8_t *buffer, int nbytes)
{
	uint32_t err_code;

	for (uint32_t i = 0; i < nbytes; i++) {
		do {
			err_code = app_uart_put(buffer[i]);
			if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
				//NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
				APP_ERROR_CHECK(err_code);
		} while (err_code == NRF_ERROR_BUSY);
	}
	return err_code;
}

void Gsm_RingBuf(uint8_t in_data)
{
	Gsm_RxBuf[rxWriteIndex] = in_data;
	rxWriteIndex++;
	rxCount++;

	if (rxWriteIndex == GSM_RXBUF_MAXSIZE)
		rxWriteIndex = 0;

	/* Check for overflow */
	if (rxCount == GSM_RXBUF_MAXSIZE) {
		rxWriteIndex = 0;
		rxCount = 0;
		rxReadIndex = 0;
	}
}


void Gsm_PowerUp(void)
{
	DPRINTF(LOG_DEBUG, "GMS_PowerUp\r\n");

	GSM_PWR_OFF;
	delay_ms(200);

	/*Pwr stable wait at least 30ms before pulling down PWRKEY pin*/
	GSM_PWR_ON;
	GSM_RESET_HIGH;
	delay_ms(60);           // >30ms

	/*Pwr key keep to low at least 100ms*/
	GSM_PWRKEY_LOW;
	delay_ms(300); //300ms
	GSM_PWRKEY_HIGH;
	delay_ms(500);
}

void Gsm_PowerDown(void)
{
#if 0
	DPRINTF(LOG_DEBUG, "GMS_PowerDown\r\n");
	GSM_PWD_LOW;
	delay_ms(800);          //800ms     600ms > t >1000ms
	GSM_PWD_HIGH;
	delay_ms(12000);        //12s
	GSM_PWR_EN_DISABLE;
	delay_ms(2000);
#endif
}

int Gsm_RxByte(void)
{
	int c = -1;

	__disable_irq();
	if (rxCount > 0) {
		c = Gsm_RxBuf[rxReadIndex];

		rxReadIndex++;
		if (rxReadIndex == GSM_RXBUF_MAXSIZE)
			rxReadIndex = 0;
		rxCount--;
	}
	__enable_irq();

	return c;
}


int Gsm_WaitRspOK(char *rsp_value, uint32_t timeout_ms, uint8_t is_rf)
{
	int retavl = -1, wait_len = 0;
	char len[10] = { 0 };
	uint32_t time_count = timeout_ms;
	uint16_t i = 0;
	int c;
	char *cmp_p = NULL;

	memset(resp, 0, 1600);
	wait_len = is_rf ? strlen(GSM_CMD_RSP_OK_RF) : strlen(GSM_CMD_RSP_OK);

	if (g_type == GSM_TYPE_FILE) {
		do {
			c = Gsm_RxByte();
			if (c < 0) {
				time_count--;
				delay_ms(1);
				continue;
			}

			rsp_value[i++] = (char)c;
			SEGGER_RTT_printf(0, "%02X", rsp_value[i - 1]);
			time_count--;
		} while (time_count > 0);
	} else {
		do {
			int c;
			c = Gsm_RxByte();
			if (c < 0) {
				time_count--;
				delay_ms(1);
				continue;
			}
			//R485_UART_TxBuf((uint8_t *)&c,1);
			SEGGER_RTT_printf(0, "%c", c);
			resp[i++] = (char)c;

			if (i >= wait_len) {
				if (is_rf)
					cmp_p = strstr(resp, GSM_CMD_RSP_OK_RF);
				else
					cmp_p = strstr(resp, GSM_CMD_RSP_OK);
				if (cmp_p) {
					if (i > wait_len && rsp_value != NULL)
						//SEGGER_RTT_printf(0,"--%s  len=%d\r\n", resp, (cmp_p-resp));
						memcpy(rsp_value, resp, (cmp_p - resp));
					retavl = 0;
					break;
				}
			}
		} while (time_count > 0);
	}

	return retavl;
}

int Gsm_WaitSendAck(uint32_t timeout_ms)
{
	int retavl = -1;
	uint32_t time_count = timeout_ms;

	do {
		int c;
		c = Gsm_RxByte();
		if (c < 0) {
			time_count--;
			delay_ms(1);
			continue;
		}
		//R485_UART_TxBuf((uint8_t *)&c,1);
		if ((char)c == '>') {
			retavl = 0;
			break;
		}
	} while (time_count > 0);

	//DPRINTF(LOG_DEBUG,"\r\n");
	return retavl;
}

int Gsm_WaitNewLine(char *rsp_value, uint32_t timeout_ms)
{
	int retavl = -1;
	int i = 0;
	uint32_t time_count = timeout_ms;
	char new_line[3] = { 0 };
	char str_tmp[GSM_GENER_CMD_LEN];
	char *cmp_p = NULL;

	memset(str_tmp, 0, GSM_GENER_CMD_LEN);
	do {
		int c;
		c = Gsm_RxByte();
		if (c < 0) {
			time_count--;
			delay_ms(1);
			continue;
		}
		//R485_UART_TxBuf((uint8_t *)&c,1);
		SEGGER_RTT_printf(0, "%c", c);
		str_tmp[i++] = c;
		if (i > strlen("\r\n")) {
			if ((cmp_p = strstr(str_tmp, "\r\n")) != NULL) {
				memcpy(rsp_value, str_tmp, (cmp_p - str_tmp));
				retavl = 0;
				break;
			}
		}
	} while (time_count > 0);

	//DPRINTF(LOG_DEBUG,"\r\n");
	return retavl;
}

void Gsm_print(char *at_cmd)
{
	uint8_t cmd_len;

	memset(GSM_CMD, 0, GSM_GENER_CMD_LEN);
	cmd_len = sprintf(GSM_CMD, "%s\r\n", at_cmd);
	GSM_UART_TxBuf((uint8_t *)GSM_CMD, cmd_len);
}

void Gsm_nb_iot_config(void)
{
	int retavl = -1;

	Gsm_print("AT+COPS=?");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	delay_ms(1000);

	Gsm_print("AT+COPS=1,0,\"CHINA MOBILE\",0");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	delay_ms(1000);

	Gsm_print("AT+QNWINFO");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 4, true);
	delay_ms(1000);

	Gsm_print("AT+QICSGP=1,1,\"CMCC\"," "," ",1");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	delay_ms(1000);

	Gsm_print("AT+QIACT=1");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	delay_ms(1000);

	Gsm_print("AT+QIACT?");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	delay_ms(1000);

	Gsm_print("AT+QIOPEN=1,1,\"TCP LISTENER\",\"127.0.0.1\",0,2020,0");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	delay_ms(1000);

	Gsm_print("AT+QISTATE");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	DPRINTF(LOG_DEBUG, "AT+QISTATE GSM_RSP = %s\r\n", GSM_RSP);
	delay_ms(1000);
}

void gps_config()
{
	int retavl = -1;
	uint8_t cmd_len;

	memset(CMD, 0, GSM_GENER_CMD_LEN);
	memset(RSP, 0, GSM_GENER_CMD_LEN);
	cmd_len = sprintf(CMD, "%s\r\n", "AT+QGPSCFG=\"gpsnmeatype\",1");
	GSM_UART_TxBuf((uint8_t *)CMD, cmd_len);
	retavl = Gsm_WaitRspOK(RSP, GSM_GENER_CMD_TIMEOUT * 4, true);
	DPRINTF(LOG_DEBUG, "AT+QGPSCFG= retavl= %d\r\n", retavl);
	delay_ms(1000);
	memset(CMD, 0, GSM_GENER_CMD_LEN);
	memset(RSP, 0, GSM_GENER_CMD_LEN);
	cmd_len = sprintf(CMD, "%s\r\n", "AT+QGPS=1,1,1,1,1");
	GSM_UART_TxBuf((uint8_t *)CMD, cmd_len);
	retavl = Gsm_WaitRspOK(RSP, GSM_GENER_CMD_TIMEOUT * 4, true);
	DPRINTF(LOG_DEBUG, "AT+QGPS retavl= %d\r\n", retavl);
	delay_ms(1000);
}

void gps_data_get()
{
	int retavl = -1;
	uint8_t cmd_len;

	memset(RSP, 0, GSM_GENER_CMD_LEN);
	Gsm_print("AT+QGPSGNMEA=\"GGA\"");
	retavl = Gsm_WaitRspOK(RSP, GSM_GENER_CMD_TIMEOUT, true);
	GpsParseGpsData_2(RSP);
	memset(gps_data, 0, 512);
	sprintf(gps_data, GPS_FORMAT, NmeaGpsData.NmeaDataType,
		NmeaGpsData.NmeaUtcTime,
		NmeaGpsData.NmeaLatitude,
		NmeaGpsData.NmeaLatitudePole,
		NmeaGpsData.NmeaLongitude,
		NmeaGpsData.NmeaLongitudePole,
		NmeaGpsData.NmeaAltitude,
		NmeaGpsData.NmeaAltitudeUnit,
		NmeaGpsData.NmeaHeightGeoid,
		NmeaGpsData.NmeaHeightGeoidUnit
		);
	DPRINTF(LOG_INFO, "%s", gps_data);
}

// Get socket state
int Gsm_GetSocketStateCmd(void)
{
	int retavl = -1;

	Gsm_print("AT+QISTATE");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
	DPRINTF(LOG_INFO, "AT+QISTATE GSM_RSP = %s\r\n", GSM_RSP);
}

// Open socket
int Gsm_OpenSocketCmd(void *context, uint8_t ServiceType, uint8_t accessMode)
{
	int retavl = -1;
	char *cmd;
	int retry_count;

	SocketContext *sock = (SocketContext *)context;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s%d,%d,\"%s\",\"%s\",%d,%d,%d\r\n", GSM_OPENSOCKET_CMD_STR, sock->contextID, sock->connectID,
				  (ServiceType == GSM_TCP_TYPE) ? GSM_TCP_STR : GSM_UDP_STR,
				  sock->addr.hostname, sock->addr.remote_port, sock->addr.local_port, accessMode);

		retry_count = 3;
		do {
			retry_count--;
			GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);
			memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
			retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 40, true);
		} while (retry_count && retavl);

		free(cmd);
	}
	return retavl;
}


//  close socket
int Gsm_CloseSocketCmd(void)
{
	int retavl = -1;
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_CLOSESOCKET_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

		free(cmd);
	}
	return retavl;
}


// send data
int Gsm_SendDataCmd(void *context, char *data, uint16_t len, uint32_t timeout)
{
	int retavl = -1;
	char *cmd;

	SocketContext *sock = (SocketContext *)context;

	connectID = sock->connectID;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s%d,%d\r\n", GSM_SENDDATA_CMD_STR, connectID, len);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);
		DPRINTF(LOG_INFO, "cmd=%s\ncmd_len=%d", cmd, cmd_len);
		DPRINTF(LOG_INFO, "data=%s\n", data);

		retavl = Gsm_WaitSendAck(GSM_GENER_CMD_TIMEOUT * 4);//500*4=2000ms
		if (retavl == 0) {
			DPRINTF(LOG_INFO, "GSM_SEND_DATA\n");
			GSM_UART_TxBuf((uint8_t *)data, len);
			memset(cmd, 0, GSM_GENER_CMD_LEN);
			retavl = Gsm_WaitRspOK(cmd, GSM_GENER_CMD_TIMEOUT, true);
			if (retavl == 0)
				retavl = len;
			else if (retavl > 0)
				if (strstr(cmd, "SEND OK"))
					retavl = 0;
		} else {
			//DPRINTF(LOG_INFO, "ret1=%d", retavl);
		}
		free(cmd);
	}
	return retavl;
}

int Gsm_WaitRecvPrompt(uint32_t timeout)
{
	int retavl = -1;
	uint8_t wait_len;
	uint32_t time_count = timeout_ms;

	memset(resp, 0, 1600);
	wait_len = strlen(GSM_RECVDATA_RSP_STR);

	do {
		int c;
		c = Gsm_RxByte();
		if (c < 0) {
			time_count--;
			delay_ms(1);
			continue;
		}
		//R485_UART_TxBuf((uint8_t *)&c,1);
		SEGGER_RTT_printf(0, "%c", c);
		resp[i++] = (char)c;

		if (i >= wait_len) {
			cmp_p = strstr(resp, GSM_RECVDATA_RSP_STR);
			if (cmp_p) {
				retavl = 0;
				break;
			}
		}
	} while (time_count > 0);

	return retavl;
}

// Read Recv Data
int Gsm_ReadRecvBufferCmd(uint8_t connectID, uint16_t len, uint32_t timeout)
{
	int retavl = -1;
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s%d,%d\r\n", GSM_READDATA_CMD_STR, connectID, len);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		retavl = Gsm_WaitRecvPrompt(timeout);

		if (retavl == 0) {
			memset(resp, 0, 1600);
			retavl = Gsm_WaitNewLine(resp, timeout);
		}

		free(cmd);
	}
	return retavl;
}

// Receive Data
uint16_t Gsm_RecvData(uint8_t connectID, char *recv_buf, uint16_t len, uint32_t timeout)
{
	int c = 0, i = 0;
	uint8_t retavl = -1;
	uint32_t time_left = timeout;

	retavl = Gsm_ReadRecvBufferCmd(connectID, len, timeout);

	if (retavl == 0) {
		DPRINTF(LOG_INFO, "Rx:");
		do {
			c = Gsm_RxByte();
			if (c < 0) {
				time_left--;
				delay_ms(1);
				continue;
			}
			//R485_UART_TxBuf((uint8_t *)&c,1);
			SEGGER_RTT_printf(0, "%c", c);
			recv_buf[i++] = (char)c;
			if (i > len)
				DPRINTF(LOG_INFO, "\r\nRx Too Many data\r\n");
		} while (time_left > 0);
		SEGGER_RTT_printf(0, "\r\n");
	}

	return i;
}

int Gsm_AutoBaud(void)
{
	int retavl = -1, rety_cunt = GSM_AUTO_CMD_NUM;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_AUTO_CMD_STR);
		do {
			DPRINTF(LOG_DEBUG, "\r\n auto baud rety\r\n");
			GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

			retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, NULL);
			delay_ms(500);
			rety_cunt--;
		} while (retavl != 0 && rety_cunt > 0);

		free(cmd);
	}
	DPRINTF(LOG_DEBUG, "Gsm_AutoBaud retavl= %d\r\n", retavl);
	return retavl;
}

int Gsm_FixBaudCmd(int baud)
{
	int retavl = -1;
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s%d%s\r\n", GSM_FIXBAUD_CMD_STR, baud, ";&W");
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

		free(cmd);
	}
	DPRINTF(LOG_DEBUG, "Gsm_FixBaudCmd retavl= %d\r\n", retavl);
	return retavl;
}

//close cmd echo
int Gsm_SetEchoCmd(int flag)
{
	int retavl = -1;
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s%d\r\n", GSM_SETECHO_CMD_STR, flag);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

		free(cmd);
	}
	DPRINTF(LOG_DEBUG, "Gsm_SetEchoCmd retavl= %d\r\n", retavl);
	return retavl;
}

//Check SIM Card Status
int Gsm_CheckSimCmd(void)
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_CHECKSIM_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(cmd, GSM_GENER_CMD_TIMEOUT, true);
		DPRINTF(LOG_DEBUG, "Gsm_CheckSimCmd cmd= %s\r\n", cmd);
		if (retavl >= 0) {
			if (NULL != strstr(cmd, GSM_CHECKSIM_RSP_OK))
				retavl = 0;
			else
				retavl = -1;
		}


		free(cmd);
	}
	DPRINTF(LOG_DEBUG, "Gsm_CheckSimCmd retavl= %d\r\n", retavl);
	return retavl;
}

void gsm_send_test(void)
{
	int retavl = -1;
	int len = 0;
	char *test = NULL;

	DPRINTF(LOG_INFO, "+++++send gps data++++");
	Gsm_print("AT+QISEND=1,75");
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT * 400, true);

	Gsm_print("$GPGGA,134303.00,3418.040101,N,10855.904676,E,1,07,1.0,418.5,M,-28.0,M,,*4A");
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT * 400, true);
	DPRINTF(LOG_DEBUG, " gps_data send retavl= %d\r\n", retavl);

	DPRINTF(LOG_INFO, "+++++send sensor data++++");
	Gsm_print("AT+QISEND=1,170");
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT * 400, true);

	Gsm_print(post_data);
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT * 400, true);
	DPRINTF(LOG_DEBUG, " sensor_data send retavl= %d\r\n", retavl);
}

int Gsm_test_hologram(void)
{
	int retavl = -1;
	int time_count;
	int ret;
	int cmd_len;
	int retry_count;

	Gsm_print("AT+COPS=?");
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

	Gsm_print("AT+COPS=1,0,\"CHINA MOBILE\",0");
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

	while ((Gsm_CheckNetworkCmd() < 0)) {
		if (++time_count > GSM_CHECKSIM_RETRY_NUM) {
			DPRINTF(LOG_WARN, "check network timeout\r\n");
			return -1;
		}
	}

	Gsm_print("AT+QNWINFO");
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT * 4, true);
	if (retavl >= 0) {
		DPRINTF(LOG_INFO, "Wait +QNWINFO RSP!\n");
		if (NULL != strstr((char *)GSM_RSP, "+QNWINFO: \"EDGE\""))
			retavl = 0;
		else
			retavl = -1;
	}
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT, true);
	if (retavl >= 0) {
		if (NULL != strstr((char *)GSM_RSP, "Hologram"))
			retavl = 0;
		else
			retavl = -1;
	}
	vTaskDelay(300);

	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	Gsm_print("AT+QICSGP=1,1,\"hologram\",\"\",\"\",1");
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT, true);

	Gsm_print("AT+QIACT=1");
	retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT * 4, true);
	DPRINTF(LOG_INFO, "AT+QIACT=1\n");

	retry_count = 3;
	do {
		retry_count--;
		Gsm_print("AT+QIACT?");
		memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT, true);
		if (retavl >= 0) {
			if (NULL != strstr((char *)GSM_RSP, "+QIACT: 1,1,1"))
				retavl = 0;
			else
				retavl = -1;
		}
	} while (retry_count && retavl);

	retry_count = 3;
	do {
		retry_count--;
		Gsm_print("AT+QIOPEN=1,0,\"TCP\",\"cloudsocket.hologram.io\",9999,0,1");
		memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT, true);
	} while (retry_count && retavl);

	retry_count = 2;
	do {
		retry_count--;
		Gsm_print("AT+QISEND=0,48");
		retavl = Gsm_WaitSendAck(GSM_GENER_CMD_TIMEOUT);
	} while (retry_count && retavl);

	if (retavl == 0) {
		DPRINTF(LOG_INFO, "------GSM_SEND_DATA\n");
		Gsm_print("{\"k\":\"+C7pOb8=\",\"d\":\"Hello,World!\",\"t\":\"TOPIC1\"}");
	}
	memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
	retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT, true);
	if (retavl >= 0) {
		if (NULL != strstr((char *)GSM_RSP, "SEND OK"))
			retavl = 0;
		else
			retavl = -1;
	}
	return retavl;
}

//Check Network register Status
int Gsm_CheckNetworkCmd(void)
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_CHECKNETWORK_CMD_STR);
		//DPRINTF(LOG_INFO, "%s", cmd);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(cmd, GSM_GENER_CMD_TIMEOUT, true);

		if (retavl >= 0) {
			if (strstr(cmd, GSM_CHECKNETWORK_RSP_OK))
				retavl = 0;
			else if (strstr(cmd, GSM_CHECKNETWORK_RSP_OK_5))
				retavl = 0;
			else
				retavl = -1;
		}


		free(cmd);
	}
	return retavl;
}

//Check GPRS  Status
int Gsm_CheckGPRSCmd(void)
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_CHECKGPRS_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(cmd, GSM_GENER_CMD_TIMEOUT, true);

		if (retavl >= 0)
			if (!strstr(cmd, GSM_CHECKGPRS_RSP_OK))
				retavl = -1;

		free(cmd);
	}
	return retavl;
}

//Set context 0
int Gsm_SetContextCmd(void)
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_SETCONTEXT_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

		free(cmd);
	}
	return retavl;
}

//Check  ATS=1
int Gsm_SetAtsEnCmd(void)
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_ATS_ENABLE_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

		free(cmd);
	}
	return retavl;
}

//Check  Rssi
int Gsm_GetRssiCmd()
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_RSSI_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(cmd, GSM_GENER_CMD_TIMEOUT, true);

		if (retavl >= 0) {
			if (!strstr(cmd, GSM_GETRSSI_RSP_OK)) {
				retavl = -1;
			} else {
				retavl = atoi(cmd + 2 + strlen(GSM_GETRSSI_RSP_OK));
				if (retavl == 0) {
					retavl = 53; //113
				} else if (retavl == 1) {
					retavl = 111;
				} else if (retavl >= 2 && retavl <= 30) {
					retavl -= 2;
					retavl = 109 - retavl * 2;
				} else if (retavl >= 31) {
					retavl = 51;
				}
			}
		}

		free(cmd);
	}
	return retavl;
}

void Gsm_CheckAutoBaud(void)
{
	uint8_t is_auto = true, i = 0;
	uint16_t time_count = 0;
	uint8_t str_tmp[64];

	delay_ms(800);
	//check is AutoBaud
	memset(str_tmp, 0, 64);

	do {
		int c;
		c = Gsm_RxByte();
		if (c <= 0) {
			time_count++;
			delay_ms(2);
			continue;
		}

		//R485_UART_TxBuf((uint8_t *)&c,1);
		if (i < 64)
			str_tmp[i++] = (char)c;

		if (i > 3 && is_auto == true) {
			if (strstr((const char *)str_tmp, FIX_BAUD_URC)) {
				is_auto = false;
				time_count = 800; //Delay 400ms
			}
		}
	} while (time_count < 1000); //time out 2000ms

	if (is_auto == true) {
		Gsm_AutoBaud();

		DPRINTF(LOG_DEBUG, "\r\n  Fix baud\r\n");
		Gsm_FixBaudCmd(GSM_FIX_BAUD);
	}
}

int Gsm_WaitRspTcpConnt(char *rsp_value, uint16_t timeout_ms)
{
	int retavl = -1, wait_len = 0;
	uint16_t time_count = timeout_ms;
	char str_tmp[GSM_GENER_CMD_LEN];
	uint8_t i = 0;

	memset(str_tmp, 0, GSM_GENER_CMD_LEN);

	wait_len = strlen(GSM_OPENSOCKET_OK_STR);
	do {
		int c;
		c = Gsm_RxByte();
		if (c < 0) {
			time_count--;
			delay_ms(1);
			continue;
		}
		//R485_UART_TxBuf((uint8_t *)&c,1);
		str_tmp[i++] = (char)c;
		if (i >= wait_len) {
			char *cmp_ok_p = NULL, *cmp_fail_p = NULL;

			cmp_ok_p = strstr(str_tmp, GSM_OPENSOCKET_OK_STR);
			cmp_fail_p = strstr(str_tmp, GSM_OPENSOCKET_FAIL_STR);
			if (cmp_ok_p) {
				if (i > wait_len && rsp_value != NULL)
					memcpy(rsp_value, str_tmp, (cmp_ok_p - str_tmp));
				retavl = 0;
				break;
			} else if (cmp_fail_p) {
				retavl = GSM_SOCKET_CONNECT_ERR;
				break;
			}
		}
	} while (time_count > 0);

//    DPRINTF(LOG_DEBUG,"\r\n");
	return retavl;
}

// Query dns server address
int Gsm_QueryDnsServer(uint8_t contextID)
{
	int retavl = -1;
	//
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s%d\r\n", GSM_QUERY_DNSSERVER_CMD_STR, contextID);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(GSM_RSP, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT, true);
		DPRINTF(LOG_INFO, "DNSQuery: %s\n", GSM_RSP);

		free(cmd);
	}
	return retavl;
}

uint32_t Gsm_gethostbyname_a(uint8_t contextID, const char *pcHostName, uint16_t timeout)
{
	int retavl = -1;
	char *cmd;

	cmd = (char *)malloc(GSM_GENER_CMD_LEN);
	if (cmd) {
		uint8_t cmd_len;
		memset(cmd, 0, GSM_GENER_CMD_LEN);
		cmd_len = sprintf(cmd, "%s\r\n", GSM_DNS_GETIP_CMD_STR);
		GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);

		memset(cmd, 0, GSM_GENER_CMD_LEN);
		retavl = Gsm_WaitRspOK(NULL, GSM_GENER_CMD_TIMEOUT, true);

		free(cmd);
	}
	return retavl;
}
/**
 * @}
 */
