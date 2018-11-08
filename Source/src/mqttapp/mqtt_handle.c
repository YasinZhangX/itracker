#include "mqtt_common.h"
#include "string.h"
//#include "driver/led.h"

static uint8_t lock_status = 0;
static uint8_t socket_onoff_status;

void  topic_Onoff_Handle(char *data, uint16_t data_len, uint8_t num);
void  topic_CSLock_Handle(char *data, uint16_t data_len, uint8_t num);

const topic_Cmd topic_Cmd_Arr[]={
	{"onoff", topic_Onoff_Handle},
	{"CSLock", topic_CSLock_Handle}
};

//char to hex
 uint8_t char_toInt( uint8_t n){
  if(n<='9'&&n>='0') return n-'0';
  if(n<='f'&&n>='a') return n-'a'+10;
  if(n<='F'&&n>='A') return n-'A'+10;
  return 0xff;
}

 void chars_toHEX(uint8_t* pHex, uint8_t* buff, uint8_t len){
  uint8_t i;
  for(i=0;i<len;i++){
    pHex[i]=0;
  }
  for(i=0;i<len;i++){
    pHex[i] |= char_toInt(buff[2*i+1]);
    pHex[i] |= char_toInt(buff[2*i])<<(1*4);
  }
}

//remoteId to hex string. for example 11397(d) -> 2C85(x),so str is 852C0000
//stm32 is little endian,so when transfer to int,it is 00002c85.
 void IdtoHexStr(uint32_t number, uint8_t* remoteid){
	char i;
	uint32_t num = number;
	for(i=0;i<8;i++){
		int temp = num%16;
		num >>= 4;
		if(i%2==0){
			if(temp<10 ){
				remoteid[i+1]=temp+'0';
			}else{
				remoteid[i+1]=temp-10+'A';
			}
		}else{
			if(temp<10 ){
				remoteid[i-1]=temp+'0';
			}else{
				remoteid[i-1]=temp-10+'A';
			}
		}
	}
}

 void char_LowertoUpper(uint8_t* str)
 {
	 uint8_t t;
	//change lower letter to upper letter
	 for(t=0; str[t]!='\0'; t++)
	 {
		if(str[t]>='a'&&str[t]<='z')
				str[t]-=32;// 'a'-'A'
		else if(str[t]=='/')
				str[t]='_';
	 }
 }

 uint8_t topic_cmdCompar(uint8_t* cmd, uint8_t* topic)
 {
	uint8_t i;
	//at first,I use "tp_len=sizeof(topic);",it is an error,because it always equals 4,it just measure the pointer's size.
	for(i=0; *cmd!='\0'; i++)
	{
		if(*cmd==topic[i])
		{
			cmd++;
			continue;
		}
		else
			return 0;
	}
	if(topic[i]!='\0')
		return 0;
	return 1;
 }

uint8_t 
mqtt_message_process(char* data, uint16_t data_len, char* topic, uint8_t topic_len)
 {
	 uint8_t i,j,tmp=0;
	 char prefix[50] = {0};
	 sprintf(prefix, "ESP8266/D/%s/", sysCfg.device_id);
	 uint8_t pre_len=strlen(prefix);
	 uint8_t topic_buff[50]={0};
	  //copy a topic for compare
	 for(i=pre_len,j=0; i<topic_len; i++,j++)
	 {
		topic_buff[j]=topic[i];
	 }
	 topic_buff[j]='\0';
	 //find right function entrance
	 uint8_t arr_len=sizeof(topic_Cmd_Arr)/sizeof(topic_Cmd_Arr[0]);
	 for(i=0; i<arr_len; i++)
	 {
		//topic=topic_buff;
		tmp=topic_cmdCompar(topic_Cmd_Arr[i].topic_Cmd_str, topic_buff);
		if(tmp>0)
		{
			topic_Cmd_Arr[i].topic_CmdFn(data, data_len, i);
			return 1;
		}
		continue;
	 }
	 return 0;
 }

void 
topic_Onoff_Handle(char *data, uint16_t data_len, uint8_t num)
{
//	cJSON *json;
//	char *out;
//	json=cJSON_Parse(data);
//	cJSON_ReplaceItemInObject(json,"type",cJSON_CreateString("ESP8266.onoff.resp"));
//	//if(!lock_status)//check children safe lock
//	//{
//		int onoff_value = cJSON_GetObjectItem(json,"data")->valueint;
//		switch(onoff_value)
//		{
//			case SOCKET_OFF:
//				socket_off();
//				cJSON_AddStringToObject(json,"status", "success");
//				onoff_value = 0;
//				break;
//			case SOCKET_ON:
//				socket_on();
//				cJSON_AddStringToObject(json,"status", "success");
//				onoff_value = 1;
//				break;
//			case SOCKET_TOGGLE:
//				if(socket_onoff_status==0)
//				{
//					socket_on();
//					onoff_value = 1;
//				}else if(socket_onoff_status==1){
//					socket_off();
//					onoff_value = 0;
//				}
//				cJSON_AddStringToObject(json,"status", "success");
//				break;
//			default:
//				cJSON_AddStringToObject(json,"status", "error");
//			break;
//		}
//		socket_onoff_status = onoff_value;
//		//according onoff set red led,1 lights on,0 lights off
//		wifi_led_timer_onoff((~onoff_value)&0x01);
//		cJSON_ReplaceItemInObject(json,"data",cJSON_CreateNumber(onoff_value));
//	out=cJSON_PrintUnformatted(json);
//	char topic[50] = {0};
//	sprintf(topic, "ESP8266/U/%s/onoff", sysCfg.device_id);
//	BY_mqtt_send(topic, strlen(out), out);//send msg
//	cJSON_Delete(json);
//	//printf("%s\n",out);
//	os_free(out);
}

void 
topic_CSLock_Handle(char *data, uint16_t data_len, uint8_t num)
{
//	cJSON *json;
//	char *out;
//	json=cJSON_Parse(data);
//	cJSON_ReplaceItemInObject(json,"type",cJSON_CreateString("ESP8266.CSLock.resp"));
//	int cslock_value = cJSON_GetObjectItem(json,"data")->valueint;
//		switch(cslock_value)
//		{
//			case CSLOCK_DISABLE:
//				cJSON_AddStringToObject(json,"status", "success");
//				cslock_value = 0;
//				break;
//			case CSLOCK_ENABLE:
//				cJSON_AddStringToObject(json,"status", "success");
//				cslock_value = 1;
//				break;
//			case CSLOCK_TOGGLE:
//				if(lock_status==0)
//				{
//					cslock_value = 1;
//				}else if(lock_status==1){
//					cslock_value = 0;
//				}
//				cJSON_AddStringToObject(json,"status", "success");
//				break;
//			case CSLOCK_QUERY://just get lock status
//				break;
//			default:
//				cJSON_AddStringToObject(json,"status", "error");
//			break;
//		}
//		lock_status = cslock_value;
//		cJSON_ReplaceItemInObject(json,"data",cJSON_CreateNumber(lock_status));
//		//response to broker
//		out=cJSON_PrintUnformatted(json);
//		char topic[50] = {0};
//		sprintf(topic, "ESP8266/U/%s/CSLock", sysCfg.device_id);
//		BY_mqtt_send(topic, strlen(out), out);//send msg
//		cJSON_Delete(json);
//		//printf("%s\n",out);
//		os_free(out);
}

void 
set_onoff_status(uint8_t onoff)
{
	socket_onoff_status = onoff;
}

uint8_t 
get_onoff_status(void)
{
	return socket_onoff_status;
}

uint8_t 
get_cslock_status(void)
{
	return lock_status;
}
