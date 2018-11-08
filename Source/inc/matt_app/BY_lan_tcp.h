#ifndef _BY2_ESP8266_TCP_H_
#define	_BY2_ESP8266_TCP_H_

#include "sim_def.h"
#include "cmsis_os.h"
//typedef struct espconn* BY_lan_tcp_client_t;
#define BY_LAN_TCP_EVENT_CONNECTION_NEW 1
#define BY_LAN_TCP_EVENT_CONNECTION_CLOSE 2

uint8_t  BY_lan_tcp_init(void);
uint8_t  BY_lan_send_tcp_netstring(BY_lan_tcp_client_t client, size_t length, uint8_t * data);

#endif
