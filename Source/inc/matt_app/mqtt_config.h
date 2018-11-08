#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

#define CFG_LOCATION		0x180	/* Please don't change or if you know what you doing */
//#define MQTT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/
#define CFG_HOLDER          	0x75201314
#define MQTT_HOST		"mqtt-dev-esp8266-v1.baiyatech.com" //change to mosquitto ip and port
#define DEFAULT_SECURITY	0
#if DEFAULT_SECURITY
#define MQTT_PORT		8883
#else
#define MQTT_PORT		1883
#endif
#define MQTT_BUF_SIZE		1024  //packet max size
#define MQTT_KEEPALIVE		30	 /*second*/

#define MQTT_CLIENT_ID		"paho_mqtt"//change to registered info
#define MQTT_USER		"baiya"    //broker user and password
#define MQTT_PASS		"baiyatech"

#define STA_SSID 		"KB229"//connect to our laboratory router
#define STA_PASS 		"KB229LAB"
#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	10	/*10 seconds*/

#define QUEUE_BUFFER_SIZE		 		2048

//#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//#define PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif // __MQTT_CONFIG_H__
