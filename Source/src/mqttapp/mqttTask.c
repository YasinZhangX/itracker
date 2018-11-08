#include "paho_common.h"
#include "os_common.h"
#include "BY_mqtt.h"
#include "string.h"
#include "stdlib.h"

//#if DEFAULT_SECURITY
//#include "openssl/ssl.h"
//#include "ssl_client_crt.h"
//#define OPENSSL_DEMO_FRAGMENT_SIZE 2048
//ssl_ca_crt_key_t ssl_cck;
//#endif

extern xTaskHandle xMqttTaskHandle;
extern xQueueHandle xQueueMqtt;

uint8_t BY_mqtt_send(char* topic, size_t length, uint8_t * data);
void BY_mqtt_msg_hook(char* topic, int length, char* data);

struct Network network;
unsigned char mqtt_sendbuf[MQTT_SENDBUF_MAX] = {0};
unsigned char mqtt_readbuf[MQTT_READBUF_MAX] = {0};

InitMsg Global_initMsg;

MQTTClient client = DefaultClient;

///**
// *publish a mqtt message,it sends a message to a queue
// *@param topic the topic name
// *@param msg the mqtt message
// *@param data_len the mqtt_msg length
// *@param qos the publish qos
// *@param retain the retain messsage flag
// *@return return status,0 success,1 failure
// */
uint8_t MQTT_Publish(char *topic, char *msg, int data_len, int qos, int retain)
{
	MqttUpMsg md;
	char* buf = (char *) BY_malloc(data_len);
	if(buf == NULL){
		log_warn("memory allocate failture");
		return MQTT_PUBLISH_FAILURE;
	}
	memcpy(buf, msg, data_len);
	md.message.payload = buf;
	md.message.payloadlen = data_len;
	md.message.dup = 0;
	md.message.qos = (enum QoS)qos;
	md.message.retained = retain;
	memset(md.topic, 0, 100);
	memcpy(md.topic, topic,strlen(topic));
	//md.topic = topic;
	//log_info("topic:%s", md.topic);
	//log_info("A message is published !!!");
	uint8_t ret = xQueueSendToBack( xQueueMqtt, &md, (portTickType)30000 );
	if(ret == pdTRUE){
		return MQTT_PUBLISH_SUCCESS;
	}
	BY_free(buf);
	return MQTT_PUBLISH_FAILURE;
}

///**
// *when the device is online,publish a online message
// *@param client the mqtt basic message struct
// */
void mqttOnlineMsg(MQTTClient *client)
{
	log_info("mqtt client online message published!");
	cJSON *in;
	char *out;
	char topic[50] = {0};
	in=cJSON_CreateObject();
	cJSON_AddStringToObject(in, "type", "update");
	cJSON_AddStringToObject(in,"dev_id", (const char*) sysCfg.device_id);
	cJSON_AddStringToObject(in, "key", "network_isonline");
	cJSON_AddNumberToObject(in, "value", 1);

	//out=cJSON_Print(in);
	out = cJSON_PrintUnformatted(in);//send raw datas
	sprintf(topic, "ESP8266/U/%s/update", sysCfg.device_id);
	MQTTMessage message;
	message.payload = out;
	message.payloadlen = strlen(out);
	message.dup = 0;
	message.qos = QOS2;
	message.retained = 1;
	MQTTPublish(client, topic, &message);
	cJSON_Delete(in);
	BY_free(out);
}

/**
 * create a tcp connect
 * @param void
 * @return the tcp connect status
 */
int mqtt_tcp_connect(void)
{
	int ret;
#if DEFAULT_SECURITY
	NetworkInitSSL(&network);
#else
	NetworkInit(&network);
#endif
#if DEFAULT_SECURITY
	ssl_cck.cacrt = ca_crt;
	ssl_cck.cacrt_len = ca_crt_len;
	ssl_cck.cert = client_crt;
	ssl_cck.cert_len = client_crt_len;
	ssl_cck.key = client_key;
	ssl_cck.key_len = client_key_len;

	ret = NetworkConnectSSL(&network, MQTT_HOST, MQTT_PORT, &ssl_cck, TLSv1_1_client_method(), SSL_VERIFY_PEER, OPENSSL_DEMO_FRAGMENT_SIZE);
#else
	ret = NetworkConnect(&network, MQTT_HOST, MQTT_PORT);
#endif
	return ret;
}

/**
 * create a mqtt client and connect to the server
 * @param initMsg the struct that contains mqtt client and will message
 * @return the connect status to the mqtt server
 */
int mqtt_client_connect(InitMsg *initMsg)
{
	int ret;
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	MQTTClientInit(initMsg->client, &network, 5000, mqtt_sendbuf, MQTT_SENDBUF_MAX, mqtt_readbuf, MQTT_READBUF_MAX);
	data.willFlag = 1;
	data.will.qos = 2;
	data.will.retained = 1;
	data.will.topicName.cstring = NULL;
	data.will.topicName.lenstring.len = strlen(initMsg->will_topic);
	data.will.topicName.lenstring.data = initMsg->will_topic;
	data.will.message.cstring = NULL;
	data.will.message.lenstring.len = strlen(initMsg->will_msg);
	data.will.message.lenstring.data = initMsg->will_msg;
	data.MQTTVersion = 3;
	data.clientID.cstring = (char *)sysCfg.device_id; // you client's unique identifier
	data.username.cstring = MQTT_USER;
	data.password.cstring = MQTT_PASS;
	data.keepAliveInterval = MQTT_KEEPALIVE; // interval for PING message to be sent (seconds)
	data.cleansession = 1;
	ret = MQTTConnect(initMsg->client, &data);
	return ret;
}

/**
 * mqtt subscribe callback function
 * @param md the struct that contains mqtt message and topic
 */
void topic_received(MessageData* md)
{
    log_info("mqtt message received!");
    BY_mqtt_msg_hook(md->topicName->lenstring.data, md->message->payloadlen, (char *)md->message->payload);
}

static uint8_t BY_mqtt_update_online(void)
{
	// build cJSON
	cJSON *in;
	in = cJSON_CreateObject();
	cJSON_AddStringToObject(in, "type",      "update");
	cJSON_AddStringToObject(in, "device_id", (const char*)sysCfg.device_id);
	cJSON_AddStringToObject(in, "key",       "network_isonline");
	cJSON_AddNumberToObject(in, "value",     1);
	cJSON_AddStringToObject(in, "gw_id",     (const char*)sysCfg.device_id);

	char *out;
	out = cJSON_PrintUnformatted(in);//send raw datas

	char topic[50] = {0};
	sprintf(topic, "BY2/U/%s/update", sysCfg.device_id);

	MQTT_Publish(topic, out, strlen(out), 0, 0);

	// free cJSON
	BY_free(out);
	cJSON_Delete(in);

	return 0;
}

#define STATE_INIT	0
#define STATE_READY	1
#define STATE_ERROR	2
static uint8_t mqtt_state = STATE_INIT;
void mqtt_task(void *pvParameters)
{
	int ret;
	InitMsg *initMsg = (InitMsg *)pvParameters;
	mqtt_state = STATE_INIT;

	for(;;)
	{
		ret = mqtt_tcp_connect();
		if (ret==0)
		{
			log_info("TCP connect to mqtt host %s successfully!",MQTT_HOST);

			log_info("mqtt client connect start!");
			ret = mqtt_client_connect(initMsg);
			if (!ret)
			{
				log_info("mqtt client connect success!");
				brokenflag = AT_SUCCESS;

				//publish online
				//mqttOnlineMsg(initMsg->client);

				// Subscribe message
				char topic[100] = {0};
				sprintf(topic, "BY2/D/%s/#", sysCfg.device_id);
				MQTTSubscribe(initMsg->client, topic, QOS0, topic_received);
				log_info("subscribe contents are:%s",topic);

				if(mqtt_state != STATE_INIT)
				{
					BY_mqtt_update_online();
				}

				mqtt_state = STATE_READY;

				while (1)
				{
					// Publish all pending messages
					MqttUpMsg md;
					if(mqtt_state == STATE_ERROR)
					{
						break;
					}
					while (xQueueReceive(xQueueMqtt, &md, 100))
					{
						ret = MQTTPublish(initMsg->client, md.topic, &(md.message));
						BY_free(md.message.payload);
						if (ret != SUCCESS)
						{
							log_warn("publish status change to:%d", ret);
							break;
						}
						log_info("topic:%s", md.topic);
						log_info("A message is published !!!");
					}
					vTaskDelay(10 / portTICK_RATE_MS);
				}
				log_warn("Connection broken, request restart.");
			}
			else
			{
				log_warn("mqtt client connect failure!");
			}
#if DEFAULT_SECURITY
			DisNetworkConnectSSL(&network);
#else
			FreeRTOS_disconnect(&network);
#endif
			log_error("the mqtt client disconnected!\n");
		}
		else
		{
			xQueueReset(xQueueMqtt);
			log_warn("TCP connect error, error code is %d", ret);
			brokenflag = AT_FAILURE;
		}
		vTaskDelay(1000 / portTICK_RATE_MS);
		
	}
}
void mqtt_rcv_task(void *pvParameters)
{
	int ret;
	InitMsg *initMsg = (InitMsg *)pvParameters;

	for(;;)
	{
		if(mqtt_state == STATE_READY)
		{
			ret = MQTTYield(initMsg->client, (initMsg->client->keepAliveInterval/3)*1000);
			if (ret == FAILURE)
			{
				log_warn("yield status:%d", ret);
				mqtt_state = STATE_ERROR;
			}
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

/**
 * initialize mqtt task and queue
 * @param initMsg the basic message for initialization
 */
void mqtt_client_init(InitMsg* initMsg)
{
	xQueueMqtt = xQueueCreate(1, sizeof(MqttUpMsg));

	xTaskCreate(mqtt_task, "mqtt", 1024, (void *)initMsg, 1, &xMqttTaskHandle);
	xTaskCreate(mqtt_rcv_task, "mqtt_rcv", 1024, (void *)initMsg, 1, &xMqttRcvTaskHandle);
}

/**
 * initialize initMsg struct
 * @param will_topic the will topic name
 * @param length the will message length
 * @param will_data the will data
 * @return default return 0
 */
uint8_t BY_mqtt_init(void)
{
	BY_conf_get(BY_CONF_GATEWAY_DEVICE_ID, sysCfg.device_id);

	Global_initMsg.client = &client;

	memset(Global_initMsg.will_msg, 0, sizeof(Global_initMsg.will_msg));
	memset(Global_initMsg.will_topic, 0, sizeof(Global_initMsg.will_topic));

	// build cJSON
	cJSON *in;
	in = cJSON_CreateObject();
	cJSON_AddStringToObject(in, "type",      "update");
	cJSON_AddStringToObject(in, "device_id", (const char*)sysCfg.device_id);
	cJSON_AddStringToObject(in, "key",       "network_isonline");
	cJSON_AddNumberToObject(in, "value",     0);
	cJSON_AddStringToObject(in, "gw_id",     (const char*)sysCfg.device_id);

	char *out;
	out = cJSON_PrintUnformatted(in);//send raw datas
	memcpy(Global_initMsg.will_msg, out, strlen(out));

	// free cJSON
	BY_free(out);
	cJSON_Delete(in);

	// build topic
	char topic[50] = {0};
	sprintf(topic, "BY2/U/%s/update", sysCfg.device_id);
	memcpy(Global_initMsg.will_topic, topic, strlen(topic));

	mqtt_client_init(&Global_initMsg);

	return 0;
}

/**
 * publish a mqtt message to the server
 * @param topic the topic name to be pulished
 * @param length the message length to be published
 * @param data the message to be published
 * @return return status 0 success, 1 failure
 */
uint8_t BY_mqtt_send(char* topic, size_t length, uint8_t * data)
{

	uint8_t ret = MQTT_PUBLISH_SUCCESS;
  if(!brokenflag){ret = MQTT_Publish(topic, (char *)data, length, QOS0, 0);}
  return ret;
}

void BY_mqtt_msg_hook(char* topic, int length, char* data)
{
	char * buf = (char* )BY_malloc(length+1);
	if(buf){
		memcpy(buf, data,length);
		buf[length] = '\0';
    log_info("the data length is %d",length);
    if(length > 70){
        log_info("the data is too long to log");
    }else{
        log_info("the data is %s",buf);
    }
    BY_north_mqtt_msg_handler(topic, length, (uint8_t*) buf);
		BY_free(buf);
	}else{
		log_error("memory error");
	}
}
