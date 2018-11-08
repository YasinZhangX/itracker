#ifndef __MQTT_COMMON_H__
#define __MQTT_COMMON_H__

#include "paho_common.h"

#define MQTT_CONNECT_SUCCESS 0
#define MQTT_CONNECT_FAILURE 1
#define MQTT_PUBLISH_SUCCESS 0
#define MQTT_PUBLISH_FAILURE 1

#define MQTT_SENDBUF_MAX 1536
#define MQTT_READBUF_MAX 2048

typedef struct _MqttUpMsg
{
	MQTTMessage message;
	char topic[100];
}MqttUpMsg;

typedef struct _InitMsg
{
	MQTTClient* client;
	char will_topic[50];
	char will_msg[300];
}InitMsg;

uint8_t BY_mqtt_init(void);
uint8_t BY_mqtt_send(char* topic, size_t length, uint8_t * data);

#endif
