#ifndef SERIAL_NUM_H
#define SERIAL_NUM_H

#include "c_types.h"

typedef struct{
	u8 sernum[30];
	u8 user[10];
}at_info;

typedef void (*at_CmdFn_t)(at_info *data);

typedef struct{
  u8* at_Cmd_str;
  at_CmdFn_t at_CmdFn;
  u8* attention_str;
}at_Cmd;


void ICACHE_FLASH_ATTR at_Uart_Handle(char *args);

#endif
