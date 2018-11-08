#include "mqtt_common.h"

#define USER "baiya"
#define SER_LOCATION 0x90

u8 resp[50] = {0};

void ICACHE_FLASH_ATTR at_Query_Handle(at_info *data);
void ICACHE_FLASH_ATTR at_Update_Handle(at_info *data);
void ICACHE_FLASH_ATTR at_SerD_Handle(at_info *data);

const at_Cmd at_Cmd_Arr[]={
	{"AT+QUERY:", at_Query_Handle, "Handle serial number query."},
	{"AT+UPDATE:", at_Update_Handle, "Handle serial number update."},
	{"AT+SERD:", at_SerD_Handle, "Handle serial number set."}
};

void split_SerNum(char *data,at_info *info)
{
	u8 i,j=0;
	u8 flag = 0;
	memset(info->user, 0, 10);
	memset(info->sernum, 0, 30);
	uint8 len = strlen(data);
	for(i=0; i<len; i++)
	{
		if(!flag)
		{
			if(data[i]!=',')
				info->user[i] = data[i];
			else{
				flag = 1;
				info->user[i] = '\0';
			}
		}else{
			if(data[i]=='\0')
			{
				info->sernum[j] = '\0';
				//flag = 0;
				break;
			}
			info->sernum[j++] = data[i];
		}
	}
}

void ICACHE_FLASH_ATTR
at_Query_Handle(at_info *data)
{
	u8 tmp = strncmp(data->user, USER ,strlen(USER));// right privilege
	if(!tmp)
	{
		if(sysCfg.device_id)
		{
			memset(resp, 0 , 50);
			sprintf(resp, "AT+RESPU:%s\n", sysCfg.device_id);
			uart0_sendStr1(resp);
			//printf("AT+RESPU:%s\r\n", sysCfg.device_id);
		}else{
			uart0_sendStr1("AT+RESPU:Error 01\r\n");
			//printf("AT+RESPU:Error 01\r\n");
		}
	}else{
		uart0_sendStr1("AT+RESPU:Error 03\r\n");
		//printf("AT+RESPU:Error 03\r\n");
	}

}

void ICACHE_FLASH_ATTR
at_Update_Handle(at_info *data)
{
	u8 tmp = strncmp(data->user, USER ,strlen(USER));// right privilege
	if(!tmp)
	{
		if(sysCfg.device_id)
		{
			memset(resp, 0 , 50);
			sprintf(resp, "AT+RESPU:The old serial num is %s\r\n", sysCfg.device_id);
			uart0_sendStr1(resp);
			//printf("AT+RESPU:The old serial num is %s\r\n", sysCfg.device_id);
			CFG_ReLoad_ID(data->sernum);
			uart0_sendStr1("AT+RESPU:Ok\r\n");
			//printf("AT+RESPU:Ok\r\n");
		}else{
			uart0_sendStr1("AT+RESPU:Error 01\r\n");
			//printf("AT+RESPU:Error 01\r\n");
		}
	}else{
		uart0_sendStr1("AT+RESPU:Error 03\r\n");
		//printf("AT+RESPU:Error 03\r\n");
	}

}

void ICACHE_FLASH_ATTR
at_SerD_Handle(at_info *data)
{
	u8 tmp = strncmp(data->user, USER ,strlen(USER));// right privilege
	if(!tmp)
	{
		CFG_ReLoad_ID(data->sernum);
		uart0_sendStr1("AT+RESPU:Ok\r\n");
		//printf("AT+RESPU:Ok\r\n");
	}else{
		uart0_sendStr1("AT+RESPU:Error 03\r\n");
		//printf("AT+RESPU:Error 03\r\n");
	}
}

void ICACHE_FLASH_ATTR
at_Uart_Handle(char *args)
{
	u8 i,j,tmp=1;
	u8 at_buff[32]={0};
	for(i=0; i<32; i++)
	 {
		at_buff[i]=args[i];
		if(args[i]==':')
		{
			i++;
			break;
		}
	 }
	 at_buff[i]='\0';
	u8 arr_len=sizeof(at_Cmd_Arr)/sizeof(at_Cmd_Arr[0]);
	for(j=0; j<arr_len; j++)
	{
	   tmp=strcmp(at_buff, at_Cmd_Arr[j].at_Cmd_str);
	   if(!tmp)
	   {
		   at_info info;
		   split_SerNum(args+i, &info);
		   at_Cmd_Arr[j].at_CmdFn(&info);
		   break;
	   }else{
		   continue;
	   }
	 }
}

