#ifndef _U_GSM_H
#define _U_GSM_H

#include "stdint.h"
#include "string.h"
#include "stdbool.h"
#include "nrf_delay.h"
#include "nrf_error.h"
#include "app_error.h"
#include "bsp_itracker.h"
#include "mqttnet.h"

#define  GSM_GENER_CMD_LEN                    (128)
#define  GSM_GENER_CMD_TIMEOUT                (500)             //ms
#define  GSM_OPENSOCKET_CMD_TIMEOUT           (15000)           //ms
#define  GSM_GETDNSIP_CMD_TIMEOUT             (30 * 1000)       //ms

#define  GSM_FIX_BAUD                         (115200)          //baud
#define  GSM_CHECKSIM_RETRY_NUM               (5)               //60S
#define  GSM_CHECKSIM_RETRY_TIME              (1000)            //1000ms

#define  GSM_AUTO_CMD_NUM                     (10)

#define  GSM_TCP_STR                          "TCP"
#define  GSM_UDP_STR                          "UDP"
#define  GSM_TCP_TYPE                         (0)
#define  GSM_UDP_TYPE                         (1)

#define  GSM_BUFFER_ACCESS_MODE               (0)
#define  GSM_DIRECT_PUSH_MODE                 (1)
#define  GSM_TRANSPARENT_ACCESS_MODE          (2)

#define  GSM_SOCKET_CONNECT_ERR               (-2)

#define  GSM_RECVDATA_RSP_STR           "+QIRD:"
#define  GSM_OPENSOCKET_OK_STR          "CONNECT OK\r\n"
#define  GSM_OPENSOCKET_FAIL_STR        "CONNECT FAIL\r\n"
#define  FIX_BAUD_URC                   "RDY\r\n"
#define  GSM_CMD_CRLF                   "\r\n"
#define  GSM_CMD_RSP_OK_RF              "OK\r\n"
#define  GSM_CMD_RSP_OK                 "OK"
#define  GSM_CHECKSIM_RSP_OK            "+CPIN: READY"

#define  GSM_CHECKNETWORK_RSP_OK        "+CREG: 0,1"
#define  GSM_CHECKNETWORK_RSP_OK_5      "+CREG: 0,5"
#define  GSM_CHECKGPRS_RSP_OK           "+CGATT: 1"
#define  GSM_GETRSSI_RSP_OK             "+CSQ: "

#define  GSM_IPSTATUS_CONNECTING        "TCP CONNECTING"
#define  GSM_AUTO_CMD_STR               "AT"
#define  GSM_FIXBAUD_CMD_STR            "AT+IPR="
#define  GSM_SETECHO_CMD_STR            "ATE"
#define  GSM_CHECKSIM_CMD_STR           "AT+CPIN?"
//#if defined (M35)
#define  GSM_CHECKNETWORK_CMD_STR       "AT+CREG?"
//#elif defined (BC95)
//#define  GSM_CHECKNETWORK_CMD_STR     "AT+CEREG?"
//#endif
#define  GSM_CHECKGPRS_CMD_STR          "AT+CGATT?"
#define  GSM_SETCONTEXT_CMD_STR         "AT+QIFGCNT=0"
#define  GSM_SETSENDECHO_CMD_STR        "AT+QISDE=0"
#define  GSM_ATS_ENABLE_CMD_STR         "ATS0=1"

#define  GSM_OPENSOCKET_CMD_STR         "AT+QIOPEN="
#define  GSM_IPSTATIC_CMD_STR           "AT+QISTAT"
#define  GSM_RSSI_CMD_STR               "AT+CSQ"
#define  GSM_SENDDATA_CMD_STR           "AT+QISEND="
#define  GSM_CLOSESOCKET_CMD_STR        "AT+QICLOSE"
#define  GSM_READDATA_CMD_STR           "AT+QIRD="

#define  GSM_QUERY_DNSSERVER_CMD_STR    "AT+QIDNSCFG="
#define  GSM_DNS_GETIP_CMD_STR          "AT+QIDNSGIP="

#define  GSM_HTTP_SETURL_CMD_STR        "AT+QHTTPURL="
#define  GSM_HTTP_GET_CMD_STR           "AT+QHTTPGET="
#define  GSM_HTTP_READ_CMD_STR          "AT+QHTTPREAD="
#define  GSM_HTTP_POST_CMD_STR          "AT+QHTTPPOST="
#define  GSM_HTTP_DL_CMD_STR            "AT+QHTTPDL="

#if defined (BC95)
#define  GSM_CHECKNBAND_CMD_STR           "AT+NBAND?"
#define  GSM_CHECKNBAND_RSP_OK            "+NBAND:"
#define  GSM_CHECKNCFG_CMD_STR            "AT+NCONFIG?"
//#define  GSM_CHECKIMEI_CMD_STR          "AT+CGSN=1"
//#define  GSM_CHECKIMSI_CMD_STR          "AT+CIMI"
#define  GSM_CHECKNCDP_CMD_STR            "AT+NCDP?"
#define  GSM_CHECKCONSTATUS_CMD_STR       "AT+CSCON?"

#endif

#define TRANSFE_EVENT_POSTDATA  (0x01)

typedef enum G_TYPE {
	GSM_TYPE_CHAR,
	GSM_TYPE_FILE,
}GSM_RECIEVE_TYPE;
/*
 *********************************************************************************************************
 *                                         EXTERN FUNCTION
 *********************************************************************************************************
 */
void delay_ms(uint32_t ms);
int  Gsm_Init(void);
void GMS_PowerUp(void);
void GMS_PowerDown(void);
void Gsm_CheckAutoBaud(void);
//cmd list
int Gsm_AutoBaud(void);
int Gsm_FixBaudCmd(int baud);
int Gsm_SetEchoCmd(int flag);

int Gsm_CheckSimCmd(void);
int Gsm_CheckNetworkCmd(void);
int Gsm_CheckGPRSCmd(void);

int Gsm_SetContextCmd(void);
int Gsm_SetAtsEnCmd(void);
int Gsm_SetSdEchoCmd(void);

int Gsm_GetRssiCmd(void);

// socket
int Gsm_OpenSocketCmd(void *context, uint8_t ServiceType, uint8_t accessMode);
int Gsm_CloseSocketCmd(void *context);

// DNS
int Gsm_SetDnsModeCmd(uint8_t contextID);

// Tx
int Gsm_SendDataCmd(void *context, const void *data, uint16_t len, uint32_t timeout_ms);
// Rx
int Gsm_ReadRecvBufferCmd(void *context, uint16_t len, uint32_t timeout);
uint16_t Gsm_RecvRawData(byte *recv_buf, uint16_t len, uint32_t timeout);

// gsm test
void gsm_send_test(void);
void Gsm_RecvData_test(char *recv_buf, uint16_t timeout);

// nbiot config
int  Gsm_nb_iot_config(void);

// gps
int  gps_config(void);
void gps_data_get(void);


#if defined (BC95)
int Gsm_CheckNbandCmd(int *band);
int Gsm_CheckConStatusCmd(void);
#endif

void ToPostTransfeData(char *buf, int len);

#endif
