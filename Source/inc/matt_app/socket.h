#ifndef _SOCKET_H_
#define _SOCKET_H_

#define SOCKET_OFF		0
#define SOCKET_ON		1
#define SOCKET_TOGGLE  	2

#define CSLOCK_DISABLE  0
#define CSLOCK_ENABLE   1
#define CSLOCK_TOGGLE   2
#define CSLOCK_QUERY	3

void ICACHE_FLASH_ATTR socket_init(void);
void ICACHE_FLASH_ATTR socket_on(void);
void ICACHE_FLASH_ATTR socket_off(void);

#endif
