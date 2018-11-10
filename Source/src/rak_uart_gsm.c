/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#include "rak_uart_gsm.h"
#include "gps.h"

#define  GSM_RXBUF_MAXSIZE           1600

static uint16_t rxReadIndex  = 0;
static uint16_t rxWriteIndex = 0;
static uint16_t rxCount      = 0;
static uint8_t Gsm_RxBuf[GSM_RXBUF_MAXSIZE];

//extern uint8_t uart2_rx_data;
extern int R485_UART_TxBuf(uint8_t *buffer, int nbytes);
extern GSM_RECIEVE_TYPE g_type;

char GSM_CMD[60] = {0};
uint8_t GSM_RSP[1600] = {0};
char resp[1500]={0};
extern char CMD[128];
extern char RSP[128];
char gps_data[512] = {0};
extern tNmeaGpsData NmeaGpsData;
extern char post_data[1024];
#define  GPS_FORMAT 				\
				"{\r\n"							\
				"\"NmeaDataType\" : %s,\r\n"				\
				"\"NmeaUtcTime\" : %s,\r\n"				\
				"\"NmeaLatitude\" : %s,\r\n"				\
				"\"NmeaLatitudePole\" : %s,\r\n"				\
				"\"NmeaLongitude\" : %s,\r\n"				\
				"\"NmeaLongitudePole\" : %s,\r\n"				\
				"\"NmeaAltitude\" : %s,\r\n"\
				"\"NmeaAltitudeUnit\" : %s,\r\n"\
				"\"NmeaHeightGeoid\" : %s,\r\n"		\
				"\"NmeaHeightGeoidUnit\" : %s,\r\n"			\
				"}\r\n"
/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  AppTaskGPRS ( void const * argument );

void delay_ms(uint32_t ms)
{
		nrf_delay_ms(ms);
}

int GSM_UART_TxBuf(uint8_t *buffer, int nbytes)
{
		uint32_t err_code;
		for (uint32_t i = 0; i < nbytes; i++)
		{
				do
				{
						err_code = app_uart_put(buffer[i]);
						if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
						{
								//NRF_LOG_ERROR("Failed receiving NUS message. Error 0x%x. ", err_code);
								APP_ERROR_CHECK(err_code);
						}
				} while (err_code == NRF_ERROR_BUSY);
		}
		return err_code;
}

void Gsm_RingBuf(uint8_t in_data)
{
		Gsm_RxBuf[rxWriteIndex]=in_data;
    rxWriteIndex++;
    rxCount++;
                      
    if (rxWriteIndex == GSM_RXBUF_MAXSIZE)
    {
        rxWriteIndex = 0;
    }
            
     /* Check for overflow */
    if (rxCount == GSM_RXBUF_MAXSIZE)
    {
        rxWriteIndex = 0;
        rxCount      = 0;
        rxReadIndex  = 0;
    }  
}

		
void Gsm_PowerUp(void)
{
    DPRINTF(LOG_DEBUG,"GMS_PowerUp\r\n");

    GSM_PWR_OFF;
    delay_ms(200);
	
    /*Pwr stable wait at least 30ms before pulling down PWRKEY pin*/
    GSM_PWR_ON;
		GSM_RESET_HIGH;
    delay_ms(60);		// >30ms
    
		/*Pwr key keep to low at least 100ms*/
    GSM_PWRKEY_LOW;
    delay_ms(300); //300ms
    GSM_PWRKEY_HIGH;
    delay_ms(500);
}

void Gsm_PowerDown(void)
{
	#if 0
    DPRINTF(LOG_DEBUG,"GMS_PowerDown\r\n");
    GSM_PWD_LOW;
    delay_ms(800); //800ms     600ms > t >1000ms  
    GSM_PWD_HIGH;
    delay_ms(12000); //12s
	  GSM_PWR_EN_DISABLE;
    delay_ms(2000);
	#endif
}

int Gsm_RxByte(void)
{
    int c = -1;

    __disable_irq();
    if (rxCount > 0)
    {
        c = Gsm_RxBuf[rxReadIndex];
				
        rxReadIndex++;
        if (rxReadIndex == GSM_RXBUF_MAXSIZE)
        {
            rxReadIndex = 0;
        }
        rxCount--;
    }
    __enable_irq();

    return c;
}


int Gsm_WaitRspOK(char *rsp_value, uint16_t timeout_ms, uint8_t is_rf)
{
    int retavl = -1, wait_len = 0;
	  char len[10] = {0};
    uint16_t time_count = timeout_ms;
    uint32_t i = 0;
	  int       c;			
		char *cmp_p = NULL;
    memset(resp, 0, 1600);
    wait_len = is_rf ? strlen(GSM_CMD_RSP_OK_RF) : strlen(GSM_CMD_RSP_OK);
       
	  if(g_type == GSM_TYPE_FILE)
		{
				do
        {
           c=Gsm_RxByte();
           if(c<0)
           {
             time_count--;
             delay_ms(1);
             continue; 
           }
 
           rsp_value[i++]=(char)c;		
					 SEGGER_RTT_printf(0, "%02X",rsp_value[i-1]);
					 time_count--;
				}while(time_count>0);
		}
		else
		{
		  do
			{
				int c;
				c=Gsm_RxByte();
				if(c<0)
				{
						time_count--;
						delay_ms(1);
						continue; 
				}
				//R485_UART_TxBuf((uint8_t *)&c,1);
				SEGGER_RTT_printf(0, "%c",c);
				resp[i++]=(char)c;
				
				if(i >= wait_len)
				{
						if(is_rf)
								cmp_p = strstr(resp, GSM_CMD_RSP_OK_RF);
						else
								cmp_p = strstr(resp, GSM_CMD_RSP_OK);
						if(cmp_p)
						{
								if(i>wait_len&&rsp_value!=NULL)
								{
										//SEGGER_RTT_printf(0,"--%s  len=%d\r\n", resp, (cmp_p-resp)); 
										memcpy(rsp_value,resp,(cmp_p-resp));
								}               
								retavl=0;
								break;
						}
				}
			}while(time_count>0);
		}

    return retavl;   
}

int Gsm_WaitSendAck(uint16_t timeout_ms)
{
    int retavl=-1;
    uint16_t time_count=timeout_ms;    
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);       
        if((char)c=='>')
        {
            retavl=0;
            break;
        }   
    }while(time_count>0);
    
    //DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

int Gsm_WaitNewLine(char *rsp_value,uint16_t timeout_ms)
{
    int retavl=-1;
		int i=0;
    uint16_t time_count=timeout_ms;   
		char new_line[3]={0};
		char str_tmp[GSM_GENER_CMD_LEN];
		char *cmp_p=NULL;
		memset(str_tmp, 0,GSM_GENER_CMD_LEN);
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);
				SEGGER_RTT_printf(0, "%c", c);
				str_tmp[i++]=c;
				if(i>strlen("\r\n"))
				{
						if((cmp_p = strstr(str_tmp, "\r\n")) != NULL)
						{
								memcpy(rsp_value, str_tmp, (cmp_p-str_tmp));
								retavl=0;
								break;
						}   				
				}

    }while(time_count>0);
    
    //DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

int Gsm_AutoBaud(void)
{
    int retavl=-1,rety_cunt=GSM_AUTO_CMD_NUM;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_AUTO_CMD_STR);
        do
        {
            DPRINTF(LOG_DEBUG,"\r\n auto baud rety\r\n");
            GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
            retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,NULL);
            delay_ms(500);
            rety_cunt--;
        }while(retavl!=0&& rety_cunt>0);
        
        free(cmd);
    }
		DPRINTF(LOG_DEBUG,"Gsm_AutoBaud retavl= %d\r\n",retavl);
    return retavl;
}

int Gsm_FixBaudCmd(int baud)
{
    int retavl=-1;
    char *cmd;
    
    cmd=(char*)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%d%s\r\n",GSM_FIXBAUD_CMD_STR,baud,";&W");
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
        
        free(cmd);
    }
	  DPRINTF(LOG_DEBUG,"Gsm_FixBaudCmd retavl= %d\r\n",retavl);
    return retavl;
}

//close cmd echo
int Gsm_SetEchoCmd(int flag)
{
    int retavl=-1;
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len = sprintf(cmd, "%s%d\r\n", GSM_SETECHO_CMD_STR, flag);
        GSM_UART_TxBuf((uint8_t *)cmd, cmd_len);
        
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
        
        free(cmd);
    }
	  DPRINTF(LOG_DEBUG,"Gsm_SetEchoCmd retavl= %d\r\n",retavl);		
    return retavl;
}
//Check SIM Card Status
int Gsm_CheckSimCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKSIM_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        DPRINTF(LOG_DEBUG,"Gsm_CheckSimCmd cmd= %s\r\n",cmd);	        
        if(retavl>=0)
        {
            if(NULL!=strstr(cmd,GSM_CHECKSIM_RSP_OK))
            {
                retavl=0;
            }  
            else
            {
                retavl=-1;
            }
        }
        
        
        free(cmd);
    }
    DPRINTF(LOG_DEBUG,"Gsm_CheckSimCmd retavl= %d\r\n",retavl);	
    return retavl;
}

void Gsm_print(char *at_cmd)
{
		uint8_t cmd_len;
	  memset(GSM_CMD, 0, GSM_GENER_CMD_LEN);
		cmd_len=sprintf(GSM_CMD, "%s\r\n", at_cmd);
		GSM_UART_TxBuf((uint8_t *)GSM_CMD, cmd_len);
}

void Gsm_nb_iot_config(void)
{
    int retavl=-1;
#if 0
    //query the info of BG96 GSM 
    Gsm_print("ATI");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"ATI retavl= %d\r\n",retavl);	
    delay_ms(1000);	
    //Set Phone Functionality
    Gsm_print("AT+CFUN?");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+CFUN? retavl= %d\r\n",retavl);
    delay_ms(1000);	
    //Query Network Information  
    Gsm_print("AT+QNWINFO");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+QNWINFO retavl= %d\r\n",retavl);
    delay_ms(1000);
    //Network Search Mode Configuration:0->Automatic,1->3->LTE only ;1->Take effect immediately
    Gsm_print("AT+QCFG=\"nwscanmode\",3,1");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+QCFG=\"nwscanmode\" retavl= %d\r\n",retavl);
    delay_ms(1000);
    //LTE Network Search Mode
    Gsm_print("AT+QCFG=\"IOTOPMODE\"");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+QCFG=\"IOTOPMODE\" retavl= %d\r\n",retavl);	
    delay_ms(1000);
    //Network Searching Sequence Configuration
    Gsm_print("AT+QCFG=\"NWSCANSEQ\"");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+QCFG=\"NWSCANSEQ\" retavl= %d\r\n",retavl);	
    delay_ms(1000);
    //Band Configuration
    Gsm_print("AT+QCFG=\"BAND\"");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 10,true);
    DPRINTF(LOG_DEBUG,"AT+QCFG=\"BAND\" retavl= %d\r\n",retavl);
    delay_ms(8000);
    //(wait reply of this command for several time)Operator Selection 
    Gsm_print("AT+COPS=?");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 200,true);
    DPRINTF(LOG_DEBUG,"AT+COPS=? retavl= %d\r\n",retavl);
    delay_ms(1000);
    //Switch on/off Engineering Mode
    Gsm_print("AT+QENG=\"SERVINGCELL\"");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+QENG=\"SERVINGCELL\" retavl= %d\r\n",retavl);
    delay_ms(1000);
    //Activate or Deactivate PDP Contexts
    Gsm_print("AT+CGACT?");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+CGACT? retavl= %d\r\n",retavl);
    delay_ms(1000);
    //Show PDP Address
    Gsm_print("AT+CGPADDR=1");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+CGPADDR=1 retavl= %d\r\n",retavl);	
    delay_ms(1000);
    //show signal strenth
    Gsm_print("AT+CSQ");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+CSQ retavl= %d\r\n",retavl);	
    delay_ms(1000);	
    //show net register status
    Gsm_print("AT+CEREG?");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
    DPRINTF(LOG_DEBUG,"AT+CEREG? retavl= %d\r\n",retavl);	
    delay_ms(1000);
		
		Gsm_print("AT+QIOPEN=1,0,\"TCP\",\"192.168.0.106\",60000,0,2");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 40,true);
    DPRINTF(LOG_DEBUG,"AT+QIOPEN=1,0,\"TCP\",\"192.168.0.106\",60000,0,2 retavl= %d\r\n",retavl);	
    delay_ms(1000);
		retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 200,true);
		delay_ms(1000);
		Gsm_print("AT+QISTATE");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 40,true);
		DPRINTF(LOG_DEBUG,"AT+QISTATE GSM_RSP = %s\r\n",GSM_RSP);
		delay_ms(1000);
    //open a socket of tcp as a client 
//    Gsm_print("AT+QIOPEN=1,1,\"TCP LISTENER\",\"127.0.0.1\",0,2020,0");
//    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
//    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 40,true);
//    DPRINTF(LOG_DEBUG,"AT+QIOPEN=1,1,\"TCP LISTENER\",\"127.0.0.1\",0,2020,0 retavl= %d\r\n",retavl);	
//    delay_ms(1000);
//		retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 200,true);
//		delay_ms(1000);
//		Gsm_print("AT+QISTATE");
//    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
//    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 40,true);
//		DPRINTF(LOG_DEBUG,"AT+QISTATE GSM_RSP = %s\r\n",GSM_RSP);
//		delay_ms(1000);

    //open a socket of tcp as a server listener because only listener can recieve update file
#endif
#if 1
    Gsm_print("AT+COPS=?");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT*40,true);
		delay_ms(1000);
		
    Gsm_print("AT+COPS=1,0,\"CHINA MOBILE\",0");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT*40, true);
		delay_ms(1000);
		
    Gsm_print("AT+QNWINFO");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP,GSM_GENER_CMD_TIMEOUT*4,true);
    delay_ms(1000);
		
		Gsm_print("AT+QICSGP=1,1,\"CMCC\","","",1");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP,GSM_GENER_CMD_TIMEOUT*40,true);
		delay_ms(1000);
		
		Gsm_print("AT+QIACT=1");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP, GSM_GENER_CMD_TIMEOUT*40,true);
		delay_ms(1000);
		
		Gsm_print("AT+QIACT?");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP,GSM_GENER_CMD_TIMEOUT*40,true);
		delay_ms(1000);
		
    Gsm_print("AT+QIOPEN=1,1,\"TCP LISTENER\",\"127.0.0.1\",0,2020,0");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP,GSM_GENER_CMD_TIMEOUT * 40,true);
    delay_ms(1000);
		
    Gsm_print("AT+QISTATE");
    memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK((char *)GSM_RSP,GSM_GENER_CMD_TIMEOUT * 40,true);
		DPRINTF(LOG_DEBUG,"AT+QISTATE GSM_RSP = %s\r\n",GSM_RSP);
		delay_ms(1000);
#endif
}

void gps_config()
{
	  int retavl=-1;
		uint8_t cmd_len;
	
		memset(CMD,0,GSM_GENER_CMD_LEN);
		memset(RSP,0,GSM_GENER_CMD_LEN);	
    cmd_len=sprintf(CMD,"%s\r\n","AT+QGPSCFG=\"gpsnmeatype\",1");
	  GSM_UART_TxBuf((uint8_t *)CMD,cmd_len);
	  retavl=Gsm_WaitRspOK(RSP,GSM_GENER_CMD_TIMEOUT*4,true);
		DPRINTF(LOG_DEBUG,"AT+QGPSCFG= retavl= %d\r\n",retavl);	
    delay_ms(1000);
		memset(CMD,0,GSM_GENER_CMD_LEN);
		memset(RSP,0,GSM_GENER_CMD_LEN);	
    cmd_len=sprintf(CMD,"%s\r\n","AT+QGPS=1,1,1,1,1");
		GSM_UART_TxBuf((uint8_t *)CMD,cmd_len);
	  retavl=Gsm_WaitRspOK(RSP,GSM_GENER_CMD_TIMEOUT*4,true);
		DPRINTF(LOG_DEBUG,"AT+QGPS retavl= %d\r\n",retavl);	
    delay_ms(1000);			
}

void gps_data_get()
{
		int retavl=-1;
		uint8_t cmd_len;
		memset(RSP,0,GSM_GENER_CMD_LEN);	
		Gsm_print("AT+QGPSGNMEA=\"GGA\"");
	  retavl=Gsm_WaitRspOK(RSP,GSM_GENER_CMD_TIMEOUT,true);	
		//char data[100] = {"+QGPSGNMEA: $GPGGA,134303.00,3418.040101,N,10855.904676,E,1,07,1.0,418.5,M,-28.0,M,,*4A"};
	  GpsParseGpsData_2(RSP);
		memset(gps_data,0,512);
		sprintf(gps_data, GPS_FORMAT,   NmeaGpsData.NmeaDataType,
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
void gsm_send_test(void)
{
	int retavl=-1;
	int len = 0;
	char *test = NULL;

//	Gsm_print("AT+QISEND=2,4");
//  retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 40,true);
//	Gsm_print("test");
//  retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 40,true);
//	DPRINTF(LOG_DEBUG," sensor_data send retavl= %d\r\n",retavl);	
	
	DPRINTF(LOG_INFO, "+++++send gps data++++");
//	len = strlen(gps_data);
//	DPRINTF(LOG_INFO, " gps data len = %d\r\n",len);	
//  test = (char *)malloc(len+1);
//	memset(test,0,len+1);
//	memcpy(test,gps_data,len);
//	memset(GSM_CMD,0,60);
//	sprintf(GSM_CMD, "%s%d", "AT+QISEND=1,",len);
//	DPRINTF(LOG_INFO, " gps data cmd = %s\r\n",GSM_CMD);	
	Gsm_print("AT+QISEND=1,75");
  retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 400,true);
	//vTaskDelay(500);	
	
	Gsm_print("$GPGGA,134303.00,3418.040101,N,10855.904676,E,1,07,1.0,418.5,M,-28.0,M,,*4A");
  retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 400,true);	
	DPRINTF(LOG_DEBUG," gps_data send retavl= %d\r\n",retavl);	
	//vTaskDelay(1000);
  //free(test);
	
	DPRINTF(LOG_INFO, "+++++send sensor data++++");
//	len = strlen(post_data);
//	DPRINTF(LOG_INFO, " sensor data len = %d\r\n",len);	
//	memset(GSM_CMD,0,60);
//	sprintf(GSM_CMD, "%s%d", "AT+QISEND=1,",len);
//	DPRINTF(LOG_INFO, " sensor data cmd = %s\r\n",GSM_CMD);	
	Gsm_print("AT+QISEND=1,170");
  retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 400,true);
	//vTaskDelay(500);	
	
	Gsm_print(post_data);
  retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 400,true);
	DPRINTF(LOG_DEBUG," sensor_data send retavl= %d\r\n",retavl);	
	//vTaskDelay(1000);
}
int Gsm_test_hologram(void)
{
  int retavl=-1;
	int time_count;
	int ret;
	int cmd_len;
	int retry_count;

	Gsm_print("AT+COPS=?");
	retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
	vTaskDelay(300);
	
	Gsm_print("AT+COPS=1,0,\"CHINA MOBILE\",0");
	retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
	vTaskDelay(300);

	  while((Gsm_CheckNetworkCmd()<0))
	  {
		  //delay_ms(GSM_CHECKSIM_RETRY_TIME);
		  vTaskDelay(300);	   
		  if(++time_count>GSM_CHECKSIM_RETRY_NUM)
		  {
			  DPRINTF(LOG_WARN,"check network timeout\r\n");
			  return -1;
		  }
	  }

		Gsm_print("AT+QNWINFO");
	  memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT * 4,true);
		if(retavl>=0)
		{
			DPRINTF(LOG_INFO ,"Wait +QNWINFO RSP!\n");
				if(NULL!=strstr(GSM_RSP,"+QNWINFO: \"EDGE\""))
				{
						retavl=0;
				}  
				else
				{
						retavl=-1;
				}
		}
			vTaskDelay(300);

//    memset(cmd,0,GSM_GENER_CMD_LEN);
//    cmd_len=sprintf(cmd,"%s\r\n","AT+COPS?");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		memset(cmd,0,GSM_GENER_CMD_LEN);
		Gsm_print("AT+COPS?");
		memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);
		if(retavl>=0)
		{
				if(NULL!=strstr(GSM_RSP,"Hologram"))
				{
						retavl=0;
				}  
				else
				{
						retavl=-1;
				}
		}
			vTaskDelay(300);

		memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
		Gsm_print("AT+QICSGP=1,1,\"hologram\",\"\",\"\",1");
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);
			vTaskDelay(300);

//    memset(cmd,0,GSM_GENER_CMD_LEN);
//    cmd_len=sprintf(cmd,"%s\r\n","AT+QIACT=1");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		memset(cmd,0,GSM_GENER_CMD_LEN);
		Gsm_print("AT+QIACT=1");
    retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT * 4,true);
		DPRINTF(LOG_INFO, "AT+QIACT=1\n");
			vTaskDelay(300);

//      memset(cmd,0,GSM_GENER_CMD_LEN);
//      cmd_len=sprintf(cmd,"%s\r\n","AT+QIACT?");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		memset(cmd,0,GSM_GENER_CMD_LEN);
		retry_count = 3;
		do
		{
			retry_count--;
			Gsm_print("AT+QIACT?");
			memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
			retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);
			//DPRINTF(LOG_INFO, "AT+QIACT?\n");
			if(retavl>=0)
			{
					if(NULL!=strstr(GSM_RSP,"+QIACT: 1,1,1"))
					{
							retavl=0;
					}  
					else
					{
							retavl=-1;
					}
			}
		}while(retry_count && retavl);
		vTaskDelay(100);

//        memset(cmd,0,GSM_GENER_CMD_LEN);
//        cmd_len=sprintf(cmd,"%s\r\n","AT+QICLOSE");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		Gsm_print("AT+QICLOSE");
//    	retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
//		DPRINTF(LOG_INFO, "AT+QICLOSE\n");
//		delay_ms(300);

//        memset(cmd,0,GSM_GENER_CMD_LEN);
//        cmd_len=sprintf(cmd,"%s\r\n","AT+QIOPEN=1,0,\"TCP\",\"23.253.146.203\",9999,0,1");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		memset(cmd,0,GSM_GENER_CMD_LEN);

		//Gsm_print("AT+QIOPEN=1,0,\"TCP\",\"23.253.146.203\",9999,0,1");
		retry_count = 3;
		do
		{
			retry_count--;
			Gsm_print("AT+QIOPEN=1,0,\"TCP\",\"cloudsocket.hologram.io\",9999,0,1");
			memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    	retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);
		}while(retry_count && retavl);
	  vTaskDelay(300);
		
		//while(Gsm_OpenSocketCmd(GSM_TCP_TYPE, "cloudsocket.hologram.io", 9999))
		/*
		ret = Gsm_OpenSocketCmd(GSM_TCP_TYPE, "23.253.146.203", 9999);
	    if(ret != 0)
	    {
	    	DPRINTF(LOG_INFO, "Create socket fail 23.253.146.203");
			Gsm_CloseSocketCmd(); 
	        return -1; 
	    }
			*/
/*
ret = Gsm_SendDataCmd("{\"k\":\"+C7pOb8=\",\"d\":\"RAK:----Hello,World!----\",\"t\":\"TOPIC1\"}\r\n",
								strlen("{\"k\":\"+C7pOb8=\",\"d\":\"RAK:----Hello,World!----\",\"t\":\"TOPIC1\"}\r\n")); 
		
		if (ret < 0) {
		  DPRINTF(LOG_ERROR, "Gsm send fail...\r\n");
		  Gsm_CloseSocketCmd();
		  return ret;	
		}
*/
//        memset(cmd,0,GSM_GENER_CMD_LEN);
//        cmd_len=sprintf(cmd,"%s\r\n","AT+QISEND=0,48");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		memset(cmd,0,GSM_GENER_CMD_LEN);
//        cmd_len=sprintf(cmd,"%s\n","{\"k\":\"+C7pOb8=\",\"d\":\"Hello,World!\",\"t\":\"TOPIC1\"}");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
//		memset(cmd,0,GSM_GENER_CMD_LEN);
		retry_count = 2;
		do
		{
			retry_count--;
			Gsm_print("AT+QISEND=0,48");
			retavl=Gsm_WaitSendAck(GSM_GENER_CMD_TIMEOUT);
		}while(retry_count && retavl);
		//DPRINTF(LOG_INFO, "AT+QISEND\n");
		vTaskDelay(300);
		if(retavl == 0)
    {
      DPRINTF(LOG_INFO, "------GSM_SEND_DATA\n");
			Gsm_print("{\"k\":\"+C7pOb8=\",\"d\":\"Hello,World!\",\"t\":\"TOPIC1\"}");
		}
		
//        cmd_len=sprintf(cmd,"%s\n","{\"k\":\"+C7pOb8=\",\"d\":\"Hello,World!\",\"t\":\"TOPIC1\"}");
//		GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);

//		DPRINTF(LOG_INFO, "%s", cmd);
		memset(GSM_RSP,0,GSM_GENER_CMD_LEN);
    retavl=Gsm_WaitRspOK(GSM_RSP,GSM_GENER_CMD_TIMEOUT,true);
		if(retavl>=0)
		{
				if(NULL!=strstr(GSM_RSP,"SEND OK"))
				{
						retavl=0;
				}  
				else
				{
						retavl=-1;
				}
		}
    return retavl;	
}

//Check Network register Status
int Gsm_CheckNetworkCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKNETWORK_CMD_STR);
        //DPRINTF(LOG_INFO, "%s", cmd);
				GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            
            if (strstr(cmd,GSM_CHECKNETWORK_RSP_OK))
            {
                retavl = 0;
            } 
            else if (strstr(cmd,GSM_CHECKNETWORK_RSP_OK_5)) 
            {
               retavl = 0;
            } 
            else {
               retavl = -1;
            }
        }
        
        
        free(cmd);
    }
    return retavl;
}


//Check GPRS  Status
int Gsm_CheckGPRSCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKGPRS_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            if(!strstr(cmd,GSM_CHECKGPRS_RSP_OK))
            {
                retavl=-1;
            }  
        }
       
        free(cmd);
    }
    return retavl;
}

//Set context 0
int Gsm_SetContextCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_SETCONTEXT_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}

//Set dns mode, if 1 domain, 
int Gsm_SetDnsModeCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_SETDNSMODE_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}
//Check  ATS=1
int Gsm_SetAtsEnCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_ATS_ENABLE_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        free(cmd);
    }
    return retavl;
}

//Check  Rssi
int Gsm_GetRssiCmd()
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_RSSI_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            
            if(!strstr(cmd,GSM_GETRSSI_RSP_OK))
            {
                retavl=-1;
            }  
            else
            {
                
                retavl=atoi(cmd+2+strlen(GSM_GETRSSI_RSP_OK));
                if (retavl == 0) 
                {
                   retavl =53; //113
                }
                else if (retavl ==1) 
                {
                     retavl =111;
                }
                else if (retavl >=2 && retavl <= 30)
                {                   
                    retavl -=2;
                    retavl = 109- retavl*2;                  
                }
                else if (retavl >= 31)
                {
                   retavl =51;
                }         
            }
        }
       
        free(cmd);
    }
    return retavl;
}

void Gsm_CheckAutoBaud(void)
{
    uint8_t  is_auto = true, i=0;
    uint16_t time_count =0;
    uint8_t  str_tmp[64];
	
    delay_ms(800); 
    //check is AutoBaud 
    memset(str_tmp, 0, 64);
	
    do
    {
        int       c;
        c = Gsm_RxByte();
        if(c<=0)
        {
            time_count++;
            delay_ms(2);
            continue;
        }
				
        //R485_UART_TxBuf((uint8_t *)&c,1);		
				if(i<64) {
           str_tmp[i++]=(char)c;
				}
	
        if (i>3 && is_auto == true)
        {
            if(strstr((const char*)str_tmp, FIX_BAUD_URC))
            {
                is_auto = false;
                time_count = 800;  //Delay 400ms          
            }  
        }
    }while(time_count<1000);  //time out 2000ms
    
    if(is_auto==true)
    {
        Gsm_AutoBaud();
       
        DPRINTF(LOG_DEBUG,"\r\n  Fix baud\r\n");
        Gsm_FixBaudCmd(GSM_FIX_BAUD);
    }  
}


int Gsm_WaitRspTcpConnt(char *rsp_value,uint16_t timeout_ms)
{
    int retavl=-1,wait_len=0;
    uint16_t time_count=timeout_ms;
    char str_tmp[GSM_GENER_CMD_LEN];
    uint8_t i=0;
    
    memset(str_tmp,0,GSM_GENER_CMD_LEN);
    
    wait_len=strlen(GSM_OPENSOCKET_OK_STR);
    do
    {
        int       c;
        c=Gsm_RxByte();
        if(c<0)
        {
            time_count--;
            delay_ms(1);
            continue;
        }
        //R485_UART_TxBuf((uint8_t *)&c,1);
        str_tmp[i++]=(char)c;
        if(i>=wait_len)
        {
            char *cmp_ok_p=NULL,*cmp_fail_p=NULL;
            
            cmp_ok_p=strstr(str_tmp,GSM_OPENSOCKET_OK_STR);
            cmp_fail_p=strstr(str_tmp,GSM_OPENSOCKET_FAIL_STR);
            if(cmp_ok_p)
            {
                if(i>wait_len&&rsp_value!=NULL)
                {
                     memcpy(rsp_value,str_tmp,(cmp_ok_p-str_tmp));
                }               
                retavl=0;
                break;
            }
            else if(cmp_fail_p)
            {
                retavl=GSM_SOCKET_CONNECT_ERR;
                break;
            }
                      
        }
    }while(time_count>0);
    
//    DPRINTF(LOG_DEBUG,"\r\n");
    return retavl;   
}

//Open socket 
int Gsm_OpenSocketCmd(uint8_t SocketType,char *ip,uint16_t DestPort)
{
    int retavl=-1;
    //
    char *cmd;
      
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s%s,\"%s\",\"%d\"\r\n",GSM_OPENSOCKET_CMD_STR,
                        (SocketType==GSM_TCP_TYPE)?GSM_TCP_STR:GSM_UDP_STR,
                         ip,DestPort);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);       
       
        if(retavl>=0)
        {   
            //recv rsp msg...
            retavl=Gsm_WaitRspTcpConnt(NULL,GSM_OPENSOCKET_CMD_TIMEOUT);           
        }
        
        free(cmd);
    }
    return retavl;
}




//  send data 
int Gsm_SendDataCmd(char *data, uint16_t len)
{
    int retavl=-1;
    //
    char *cmd;
      
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s0,%d\r\n",GSM_SENDDATA_CMD_STR,len);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
		DPRINTF(LOG_INFO, "cmd=%s\ncmd_len=%d", cmd, cmd_len);
		DPRINTF(LOG_INFO, "data=%s\n", data);
        
        retavl = Gsm_WaitSendAck(GSM_GENER_CMD_TIMEOUT*4);//500*4=2000ms
        if(retavl == 0)
        {
        	DPRINTF(LOG_INFO, "GSM_SEND_DATA\n");
            GSM_UART_TxBuf((uint8_t *)data,len);
			memset(cmd,0,GSM_GENER_CMD_LEN);
            retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true); 
            if(retavl == 0)
            {
                retavl = len;
            } 
			else if(retavl > 0)
			{
				if (strstr(cmd, "SEND OK"))
	            {
	                retavl = 0;
	            } 
			}
        }  
		else
		{
				//DPRINTF(LOG_INFO, "ret1=%d", retavl);
		}
        free(cmd);
    }
    return retavl;
}


uint16_t Gsm_RecvData(char *recv_buf,uint16_t block_ms)
{
      int         c=0, i=0;      
      //OS_ERR      os_err; 
      //uint32_t    LocalTime=0, StartTime=0;
            
      int time_left = block_ms;
      
      do
      {
          c= Gsm_RxByte();
          if (c < 0) 
          {                 
               time_left--;
               delay_ms(1); 			 
               continue;
          }
          //R485_UART_TxBuf((uint8_t *)&c,1);
					SEGGER_RTT_printf(0, "%c", c);
          recv_buf[i++] =(char)c;
          
      }while(time_left>0);
      
      return i;
}

//  close socket 
int Gsm_CloseSocketCmd(void)
{
    int retavl=-1;
    char *cmd;
      
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CLOSESOCKET_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        retavl=Gsm_WaitRspOK(NULL,GSM_GENER_CMD_TIMEOUT,true);
        
        free(cmd);
    }
    return retavl;
}

#if defined (BC95)

int Gsm_CheckNbandCmd(int *band)
{
    int retavl=-1;
    char *cmd;
		
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKNBAND_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        if(retavl>=0)
        {
            
            if(!strstr(cmd,GSM_CHECKNBAND_RSP_OK))
            {
                retavl=-1;
            }  
            else
            {
                *band=atoi(cmd+2+strlen(GSM_GETRSSI_RSP_OK));   
            }
        }
        free(cmd);
    }
    return retavl;
}

int Gsm_CheckConStatusCmd(void)
{
    int retavl=-1;
    //
    char *cmd;
    
    cmd=(char *)malloc(GSM_GENER_CMD_LEN);
    if(cmd)
    {
        uint8_t cmd_len;
        memset(cmd,0,GSM_GENER_CMD_LEN);
        cmd_len=sprintf(cmd,"%s\r\n",GSM_CHECKCONSTATUS_CMD_STR);
        GSM_UART_TxBuf((uint8_t *)cmd,cmd_len);
        
        memset(cmd,0,GSM_GENER_CMD_LEN);
        retavl=Gsm_WaitRspOK(cmd,GSM_GENER_CMD_TIMEOUT,true);
        
        if(retavl>=0)
        {
            
            if (strstr(cmd,GSM_CHECKCONSTATUS_RSP_OK))
            {
                retavl = 0;
            }
            else {
               retavl = -1;
            }
        }
        
        
        free(cmd);
    }
    return retavl;
}

#endif
/**
* @}
*/





