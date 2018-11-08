#ifndef MQTT_HANDLE_H_
#define MQTT_HANDLE_H_

#include "mqtt_common.h"

typedef void (*topic_CmdFn_t)(char *data, uint16_t data_len, uint8_t num);
typedef struct{
  uint8_t* topic_Cmd_str;
  topic_CmdFn_t topic_CmdFn;
}topic_Cmd;

uint8_t mqtt_message_process(char* data, uint16_t data_len, char* topic, uint8_t topic_len);

uint8_t get_onoff_status(void);
uint8_t get_cslock_status(void);

#endif
