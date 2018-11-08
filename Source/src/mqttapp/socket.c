#include "mqtt_common.h"

void ICACHE_FLASH_ATTR
socket_on(void)
{
	gpio16_output_set(1);
}

void ICACHE_FLASH_ATTR
socket_off(void)
{
	gpio16_output_set(0);
}

void ICACHE_FLASH_ATTR
socket_init(void)
{
	gpio16_output_conf();
	gpio16_output_set(1);
}
