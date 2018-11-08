#include "mqtt_common.h"
#include "netstring.h"
#include "sim_def.h"
#include "BY_log.h"
#include "common_def.h"
#include "cJSON.h"


//#define BUFFLEN 300
#define TCP_PORT_SERV 8001
#define TCP_EVENT_CONNECT_NEW	1
#define TCP_EVENT_CONNECT_CLOSE 2

//static struct espconn *sCon;

struct _tcp_client_t{
  char* data;
  BY_lan_tcp_client_t econ;
  struct _tcp_client_t* next;
};
typedef struct _tcp_client_t  tcp_client_t;
tcp_client_t* clientList = NULL;

void BY_lan_tcp_connection_hook(int event, void* data);
void BY_lan_tcp_msg_hook(BY_lan_tcp_client_t client, size_t length, uint8_t* data);

//static void addClientByClient(tcp_client_t* client);
static uint8_t delClientByClientId(BY_lan_tcp_client_t client);
//static tcp_client_t* containClientById(BY_lan_tcp_client_t client);

/**
 * tcp connect callback function
 * @param arg the espconn struct
 */
void lan_tcp_connect(void *arg)
 {
//	struct espconn *eCon = (struct espconn *)arg;
//	BY_lan_tcp_connection_hook(TCP_EVENT_CONNECT_NEW, arg);
//	//sCon->proto.tcp->remote_port=eCon->proto.tcp->remote_port;
//	//memcpy(sCon->proto.tcp->remote_ip, eCon->proto.tcp->remote_ip, strlen(eCon->proto.tcp->remote_ip));
//	log_info("remote_port:%d", eCon->proto.tcp->remote_port);
//	log_info("remote_ip:%d.%d.%d.%d", eCon->proto.tcp->remote_ip[0], eCon->proto.tcp->remote_ip[1], eCon->proto.tcp->remote_ip[2], eCon->proto.tcp->remote_ip[3]);
 }

/**
 * tcp disconnect callback function
 * @param arg the espconn struct
 */
void lan_tcp_discon(void *arg)
{
  struct espconn *eCon = (struct espconn *)arg;
  BY_lan_tcp_connection_hook(TCP_EVENT_CONNECT_CLOSE, arg);
  delClientByClientId(eCon);
  log_info("clear disconnect client.");
}

//static tcp_client_t* containClientById(BY_lan_tcp_client_t client)
//{
//  if(clientList == NULL)
//   {
//     return NULL;
//   }
//  tcp_client_t* temp = clientList;
//  for(; temp != NULL; temp = temp->next){
//	  if(temp->econ->proto.tcp->remote_ip[0] == client->proto.tcp->remote_ip[0] &&
//	    temp->econ->proto.tcp->remote_ip[1] == client->proto.tcp->remote_ip[1] &&
//	    temp->econ->proto.tcp->remote_ip[2] == client->proto.tcp->remote_ip[2] &&
//	    temp->econ->proto.tcp->remote_ip[3] == client->proto.tcp->remote_ip[3]){
//		  return temp;
//	  }
//  }
//  return NULL;
//}

//static void addClientByClient(tcp_client_t* client)
//{
//  client->next = NULL;
//  if(clientList == NULL)
//  {
//      clientList = client;
//  }else{
//      tcp_client_t* temp = clientList;
//      while(temp->next != NULL)
//      {
//          temp = temp->next;
//      }
//      temp->next = client;
//  }
//}

static uint8_t delClientByClientId(BY_lan_tcp_client_t client)
{
//  if(clientList == NULL)
//  {
//    return FALSE;
//  }
//  tcp_client_t* temp = clientList;
//  if(clientList->econ->proto.tcp->remote_ip[0] == client->proto.tcp->remote_ip[0] &&
//    clientList->econ->proto.tcp->remote_ip[1] == client->proto.tcp->remote_ip[1] &&
//    clientList->econ->proto.tcp->remote_ip[2] == client->proto.tcp->remote_ip[2] &&
//    clientList->econ->proto.tcp->remote_ip[3] == client->proto.tcp->remote_ip[3])
//  {
//	  clientList = clientList->next;
//	  BY_free(temp->data);
//	  BY_free(temp->econ->proto.tcp);
//	  BY_free(temp->econ);
//	  BY_free(temp);
//	  printf("delete tcp client msg.\n");
//	  return TRUE;
//  }
//  while(temp != NULL)
//  {
//      tcp_client_t* cur = temp;
//      temp = temp->next;
//      if(temp->econ->proto.tcp->remote_ip[0] == client->proto.tcp->remote_ip[0] &&
//	temp->econ->proto.tcp->remote_ip[1] == client->proto.tcp->remote_ip[1] &&
//	temp->econ->proto.tcp->remote_ip[2] == client->proto.tcp->remote_ip[2] &&
//	temp->econ->proto.tcp->remote_ip[3] == client->proto.tcp->remote_ip[3])
//      {
//	  cur = temp->next;
//	  BY_free(temp->data);
//	  BY_free(temp->econ->proto.tcp);
//	  BY_free(temp->econ);
//	  BY_free(temp);
//	  log_info("delete tcp client msg.");
//	  return TRUE;
//      }
//  }
  return FALSE;
}

void 
lan_recv_tcp_msg1(void *arg, char *pdata, unsigned short len)
{
//	BY_lan_tcp_client_t eCon = (struct espconn *)arg;
//	char *netstring = NULL;
//	size_t netstring_len = 0;
//	char *tempBuff, *location;
//	tcp_client_t* client = containClientById(eCon);
//	if(client != NULL)
//	{
//	    location = tempBuff = (char *)BY_malloc(sizeof(char) * len + strlen(client->data) + 1);
//	    memset(tempBuff, 0, sizeof(char) * len + strlen(client->data) + 1);
//	    strcpy(tempBuff, client->data);
//	}else{
//	    location = tempBuff = (char *)BY_malloc(sizeof(char) * len + 1);
//	    memset(tempBuff, 0, sizeof(char) * len + 1);
//	}
//	strcat(tempBuff, pdata);
//	log_info("strcat data:%s", tempBuff);
//	int retval = netstring_read(tempBuff, strlen(tempBuff), &netstring, &netstring_len);
//	while(1)
//	{
//		if(netstring_len == 0)//parse error
//		{
//			if(retval == NETSTRING_ERROR_TOO_LONG || retval == NETSTRING_ERROR_LEADING_ZERO
//					|| retval == NETSTRING_ERROR_NO_LENGTH)
//			{
//				//printf("retval:%d\n",retval);
//				log_warn("netstring error!!!");
//				delClientByClientId(eCon);
//			}else{
//			    if(client == NULL){
//				client = (tcp_client_t*)BY_malloc(sizeof(tcp_client_t));
//				client->data = (char*)BY_malloc(strlen(tempBuff));
//				strcpy(client->data, tempBuff);
//				client->econ = (struct espconn*)BY_malloc(sizeof(struct espconn));
//				client->econ->type = ESPCONN_TCP;
//				client->econ->state = ESPCONN_NONE;
//				client->econ->proto.tcp = (esp_tcp*)BY_malloc(sizeof(esp_tcp));
//				client->econ->proto.tcp->remote_port = eCon->proto.tcp->remote_port;
//				memcpy(client->econ->proto.tcp->remote_ip, eCon->proto.tcp->remote_ip, sizeof(eCon->proto.tcp->remote_ip));
//				addClientByClient(client);
//			    }else{
//				client->data = (char*)BY_realloc(client->data, strlen(tempBuff));
//				strcpy(client->data, tempBuff);
//			    }
//			}
//			break;
//		}else{
//			BY_lan_tcp_msg_hook(eCon, netstring_len, (uint8*)netstring);
//			if(strlen(netstring + netstring_len + 1) > 0)
//			{
//			    tempBuff = netstring + netstring_len + 1;
//			    log_info("netstring:%s", netstring);
//			    retval = netstring_read(tempBuff, strlen(tempBuff), &netstring, &netstring_len);
//			}else{
//			    uint8 val = delClientByClientId(eCon);
//			    log_info("val:%d", val);
//			    break;
//			}
//		}
//	}
//	BY_free(location);
	//clearInvalidClient();//clear invaild client
}
/**
 * tcp receive message callback function
 * @param args the espconn struct
 * @param pdata received message
 * @param len the length of received message
 */
void 
lan_recv_tcp_msg(void *arg, char *pdata, unsigned short len)
 {
	struct espconn *eCon = (struct espconn *)arg;
	char *netstring = NULL;
	char *tempBuff;
	//temp = (char *)BY_malloc(sizeof(char) * len);
	tempBuff = (char *)BY_malloc(sizeof(char) * len);
	size_t netstring_len = 0;
	//printf("len:%d\n", len);
	int retval = netstring_read(pdata, len, &netstring, &netstring_len);
	while(1)
	{
		if(netstring_len == 0)//parse error
		{
			if(retval == NETSTRING_ERROR_TOO_LONG || retval == NETSTRING_ERROR_LEADING_ZERO
					|| retval == NETSTRING_ERROR_NO_LENGTH)
			{
				//printf("retval:%d\n",retval);
				log_warn("netstring error!!!");
			}
			break;
		}else{
			BY_lan_tcp_msg_hook(eCon, netstring_len, (uint8_t*)netstring);
			if(strlen(netstring + netstring_len + 1) > 0)
			{
				//copy remained data
				memset(tempBuff, 0, len);
				strcpy(tempBuff, netstring + netstring_len + 1);
				//parse remained data
				memset(pdata, 0, len);
				strcpy(pdata, tempBuff);
				//printf("netstring:%s\n", netstring);
				retval = netstring_read(pdata, strlen(pdata), &netstring, &netstring_len);
			}else{
				break;
			}
		}
	}
	BY_free(tempBuff);
 }

/**
 * initialize tcp espconn struct and listen client
 * @return default return 0
 */
uint8_t BY_lan_tcp_init(void)
 {
//	sCon=(struct espconn *)zalloc(sizeof(struct espconn));
//	sCon->type = ESPCONN_TCP;
//	sCon->state = ESPCONN_NONE;
//	sCon->proto.tcp = (esp_tcp *)zalloc(sizeof(esp_tcp));
//	sCon->proto.tcp->local_port = TCP_PORT_SERV;

//	//uint8 num = espconn_tcp_get_max_con_allow(sCon);
//	//log_info("num=%d",num);//244

//	//register TCP server timeout 60s when they don't have data to send.
//	espconn_regist_connectcb(sCon, lan_tcp_connect);
//	espconn_regist_disconcb(sCon, lan_tcp_discon);
//	//espconn_regist_recvcb(sCon, lan_recv_tcp_msg);
//	espconn_regist_recvcb(sCon, lan_recv_tcp_msg1);
//	uint8_t flag=espconn_accept(sCon);
//	if(!flag)
//	{
//		espconn_regist_time(sCon, 60, 0);
//		log_info("ESP8266 TCP connect begin...");
//	}else{
//		log_info("tcp flag = %d",flag);
//	}
	return 0;
 }

/**
 * send tcp netstring data
 * @param client espconn struct for tcp
 * @param length netstring data length
 * @param netstring data
 * @return return send data status
 */
uint8_t BY_lan_send_tcp_netstring(BY_lan_tcp_client_t client, size_t length, uint8_t * data)
{
//	char *str = NULL;
//	size_t str_length;
//	str_length = netstring_encode_new(&str, data, length);
//	uint8 ret = espconn_send(client, str, str_length);
//	BY_free(str);
	uint8_t ret = 0;
	return ret;
}

#include "core/BY_north_inf.h"

void BY_lan_tcp_connection_hook(int event, void* data)
{
    //call north handle with para event and client
    BY_north_lan_tcp_connection_handler(event, data);
}

void BY_lan_tcp_msg_hook(BY_lan_tcp_client_t client, size_t length, uint8_t* data)
{
    //printf("the data is %s\n",data);
    BY_north_lan_tcp_msg_handler(client, length, data);
}
