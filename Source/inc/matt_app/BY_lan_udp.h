#ifndef _BY2_ESP8266_UDP_H_
#define	_BY2_ESP8266_UDP_H_
#include "cmsis_os.h"
uint8_t BY_lan_udp_init(void);
uint8_t BY_lan_send_udp_broadcast (int port, int length, char * data);

#endif
