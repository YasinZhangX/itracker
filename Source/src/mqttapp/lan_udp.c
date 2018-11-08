#include "mqtt_common.h"

//static struct espconn *uCon;
#define UDP_PORT_SERV 8001

void BY_lan_udp_broadcast_msg_hook(size_t length, uint8_t* data);

/**
 *callback function of send message
 *@param arg the point of ecpconn struct
 */
void  user_udp_sent_cb(void *arg)
{
  printf("udp data sent.\n");
}

/**
 *callback function of receiving data
 *@param arg the point of ecpconn struct
 *@param pdata the point of message
 *@param len the message length
 */
void  user_udp_recv_cb(void *arg, char *pdata, unsigned short len)
{
	BY_lan_udp_broadcast_msg_hook(len,(uint8_t*) pdata);
	printf("udp receive:%s\n", pdata);
}

/**
 *initialize related data of udp struct
 *@param void
 *@return default return 0
 */
uint8_t BY_lan_udp_init(void)
{
//    const char udp_remote_ip[4] = {255,255,255,255};
//    uCon = (struct espconn *)zalloc(sizeof(struct espconn));
//    uCon->type = ESPCONN_UDP;
//    uCon->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
//    uCon->proto.udp->local_port = UDP_PORT_SERV;

//    memcpy(uCon->proto.udp->remote_ip, udp_remote_ip, 4);

//    espconn_regist_recvcb(uCon, user_udp_recv_cb);
//    espconn_regist_sentcb(uCon, user_udp_sent_cb);
//    espconn_create(uCon);
//    log_info("ESP8266 udp connect begin...");

    return 0;
}

/**
 *send a udp broadast
 *@param port the remote port
 *@param length length of the  message
 *@param data the point of message
 *@return the status of sending data
 */
uint8_t BY_lan_send_udp_broadcast (int port, int length, char * data)
{
//    uCon->proto.udp->remote_port = port;
//    uint8 retval = espconn_send(uCon, data, length);
		uint8_t retval = 0;
    return retval;
}

#include "core/BY_north_inf.h"

void BY_lan_udp_broadcast_msg_hook(size_t length, uint8_t* data)
{
    BY_north_lan_udp_msg_handler(length, data);
}
