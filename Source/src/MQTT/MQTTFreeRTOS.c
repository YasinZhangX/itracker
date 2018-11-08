/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - convert to FreeRTOS
 *******************************************************************************/

#include "MQTTFreeRTOS.h"

int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
	int rc = 0;
	uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
	UBaseType_t uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

	rc = xTaskCreate(fn,	/* The function that implements the task. */
		"MQTTTask",			/* Just a text name for the task to aid debugging. */
		usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
		arg,				/* The task parameter, not used in this case. */
		uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
		&thread->task);		/* The task handle is not used. */

	return rc;
}


void MutexInit(Mutex* mutex)
{
	mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
	return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int MutexUnlock(Mutex* mutex)
{
	return xSemaphoreGive(mutex->sem);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
	timer->xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}


void TimerCountdown(Timer* timer, unsigned int timeout) 
{
	TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(Timer* timer) 
{
	xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
	//return (timer->xTicksToWait < 0) ? 0 : (timer->xTicksToWait * portTICK_PERIOD_MS);
	/**
	* caution!!!!!!!!!!!!!!!
	*	may be buggy, when timer->xTicksToWait is signed.
	*/
	return timer->xTicksToWait * portTICK_PERIOD_MS;
}


char TimerIsExpired(Timer* timer)
{
	return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}


void TimerInit(Timer* timer)
{
	timer->xTicksToWait = 0;
	memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}

static inline int _FreeRTOS_read(unsigned char* buffer, int len, TickType_t xTicksToWait,TimeOut_t xTimeOut)
{
	static int size_remaining = 0;
	static IPD_t *mqttmsg_rcv = NULL;

	if(size_remaining == 0)
	{
		//need to fetch a new message from queue
		BaseType_t err;
		mqttmsg_rcv = NULL;
		err = xQueueReceive(IPD_Queue, &mqttmsg_rcv, xTicksToWait*portTICK_PERIOD_MS);
		if(err==pdPASS)
		{
			log_info("------------------Queue IPD get ok--------");
			size_remaining = mqttmsg_rcv->len;
		}
		else
		{
			if(err != errQUEUE_EMPTY)log_warn("------------------Queue IPD get fail------");
			buffer[0] = 0x00;
			return 0;
		}
	}

	if(len > size_remaining)
	{
		len = size_remaining;
		log_warn("Some serial bytes may have lost");
	}

	for(int i=0; i<len; i++)
	{
		buffer[i] = mqttmsg_rcv->data[i+(mqttmsg_rcv->len - size_remaining)];
	}

	size_remaining -= len;
	if((size_remaining == 0) && mqttmsg_rcv) BY_free(mqttmsg_rcv);

	return len;
}
int FreeRTOS_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	TimeOut_t xTimeOut;
	int recvLen = 0;

	vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
	do
	{
		int rc = 0;		

		rc = _FreeRTOS_read(buffer + recvLen, len - recvLen, xTicksToWait, xTimeOut);

		if (rc > 0)
			recvLen += rc;
		else if (rc < 0)
		{
			recvLen = rc;
			break;
		}
	} while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

	return recvLen;
}


static inline int _FreeRTOS_write(unsigned char* buffer, int len, TickType_t xTicksToWait,TimeOut_t xTimeOut)
{
	HAL_StatusTypeDef err;
	FSM_simple_t transtate;
	int ret = 0;
	char buf[25];

/*************************judge if data length is too long*************************/
	if(len > 1380)
	{
		len = 1380;
		log_warn("too long to publish this packet!!!");
	}

/*************************start to send data*************************/
	xQueueReset(FSM_Simple_Queue);
	sprintf(buf, "AT+CIPSENDEX=%d\r\n", len);
	err = HAL_UART_Transmit(&huart4, (uint8_t *)buf, strlen(buf), HAL_UART_COMMON_TIMEOUT);
	if(err!=HAL_OK)
	{
		log_warn("UART broken before publish!!!");
		goto fail;
	}
	do
	{
		ret = xQueueReceive(FSM_Simple_Queue, &transtate, xTicksToWait);
		if(ret==pdTRUE)
		{
			if(transtate.type == FSM_SEND_DAYU) break;
		}
		else
		{
			// do nothing
		}
	} while (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);//easy timer,but it recommend to set this inline function into freertoswrite().
	if(transtate.type != FSM_SEND_DAYU)
	{
		log_error("wait for '>' failed!!!: %d %d %d", xTimeOut.xOverflowCount, xTimeOut.xTimeOnEntering, xTicksToWait);
		goto fail;
	}

/*************************is sending data*************************/
	xQueueReset(FSM_Simple_Queue);
	err = HAL_UART_Transmit(&huart4, buffer, len, xTicksToWait*portTICK_PERIOD_MS);
	if(err!=HAL_OK)
	{
		log_warn("UART broken when publish!!!");
		goto fail;
	}

/*************************finish sending data*************************/
	do
	{
		ret = xQueueReceive(FSM_Simple_Queue, &transtate, xTicksToWait);
		if(ret==pdTRUE)
		{
			if(transtate.type == FSM_SEND_OK) break;
		}
		else
		{
			// do nothing
		}
	} while(xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
	if(transtate.type != FSM_SEND_OK)
	{
		log_error("send failed");
		goto fail;
	}

	return len;

fail:
	log_error("some trouble with mqtt publish");
	return 0;
}

int FreeRTOS_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	TimeOut_t xTimeOut;
	int sentLen = 0;

	static BY_mutex_t Mutex_write = NULL;
	if(Mutex_write == NULL) Mutex_write = BY_mutex_create();

	BY_mutex_lock(Mutex_write);

	vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
	do
	{
		int rc = 0;

		rc = _FreeRTOS_write(buffer + sentLen, len - sentLen, xTicksToWait, xTimeOut);

		if (rc > 0)
			sentLen += rc;
		else if (rc < 0)
		{
			sentLen = rc;
			break;
		}
	} while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

	BY_mutex_unlock(Mutex_write);

	return sentLen;
}


void FreeRTOS_disconnect(Network* n)
{
	HAL_StatusTypeDef err;
	xQueueReset(FSM_Simple_Queue);
	err = HAL_UART_Transmit(&huart4, AT_CIPCLOSE, strlen(AT_CIPCLOSE), HAL_UART_COMMON_TIMEOUT);
	if(err!=HAL_OK)
	{
		log_warn("UART broken when disconnect");
	}

	FSM_simple_t temp;
	BaseType_t ret;
	TimeOut_t xTimeOut;
	TickType_t xTicksToWait = 2000 / portTICK_PERIOD_MS;
	vTaskSetTimeOutState(&xTimeOut);
	do
	{
		ret = xQueueReceive(FSM_Simple_Queue, &temp, 500);
		if(ret==pdTRUE)
		{
			switch(temp.type)
			{
				case FSM_CLOSED:
					log_info("TCP close success");
					break;

				case FSM_ERROR:
					log_info("TCP close failure, maybe not connected before");
					break;

				default:
					log_warn("unconsidered return of ESP8266");
					break;
			}
		}
		else
		{
			// do nothing
		}
	} while(xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);
}


void NetworkInit(Network* n)
{
	n->my_socket = 0;
	n->mqttread = FreeRTOS_read;
	n->mqttwrite = FreeRTOS_write;
	n->disconnect = FreeRTOS_disconnect;
}


/**
return:
	0 : success
	-1: timeout
	-2: closed
*/
int NetworkConnect(Network* n, char* addr, int port)
{
	int retVal = -1;

	HAL_StatusTypeDef err;
	xQueueReset(FSM_Simple_Queue);
	err = HAL_UART_Transmit(&huart4, AT_START_TCP_CONNECT, strlen(AT_START_TCP_CONNECT), HAL_UART_COMMON_TIMEOUT);
	if(err!=HAL_OK)
	{
		log_warn("UART broken when creating TCP link");
	}

	FSM_simple_t temp;
	BaseType_t ret;
	TimeOut_t xTimeOut;
	TickType_t xTicksToWait = 5000 / portTICK_PERIOD_MS;
	vTaskSetTimeOutState(&xTimeOut);
	do
	{
		ret = xQueueReceive(FSM_Simple_Queue, &temp, 500);
		if(ret==pdTRUE)
		{
			switch(temp.type)
			{
				case FSM_TCP_CONNECT:
				case FSM_ALREADY_CONNECTED:	
					log_info("TCP connected");
					retVal = 0;
					goto exit;
	
				case FSM_CLOSED:
					log_error("Link cannot be created, maybe TCP Server has not started");
					retVal = -2;
					goto exit;

				default:
					log_warn("unconsidered return of ESP8266");
					break;
			}
		}
		else
		{
			// do nothing
		}
	} while(xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

exit:
	return retVal;
}


#if 0
int NetworkConnectTLS(Network *n, char* addr, int port, SlSockSecureFiles_t* certificates, unsigned char sec_method, unsigned int cipher, char server_verify)
{
	SlSockAddrIn_t sAddr;
	int addrSize;
	int retVal;
	unsigned long ipAddress;

	retVal = sl_NetAppDnsGetHostByName(addr, strlen(addr), &ipAddress, AF_INET);
	if (retVal < 0) {
		return -1;
	}

	sAddr.sin_family = AF_INET;
	sAddr.sin_port = sl_Htons((unsigned short)port);
	sAddr.sin_addr.s_addr = sl_Htonl(ipAddress);

	addrSize = sizeof(SlSockAddrIn_t);

	n->my_socket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
	if (n->my_socket < 0) {
		return -1;
	}

	SlSockSecureMethod method;
	method.secureMethod = sec_method;
	retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECMETHOD, &method, sizeof(method));
	if (retVal < 0) {
		return retVal;
	}

	SlSockSecureMask mask;
	mask.secureMask = cipher;
	retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &mask, sizeof(mask));
	if (retVal < 0) {
		return retVal;
	}

	if (certificates != NULL) {
		retVal = sl_SetSockOpt(n->my_socket, SL_SOL_SOCKET, SL_SO_SECURE_FILES, certificates->secureFiles, sizeof(SlSockSecureFiles_t));
		if (retVal < 0)
		{
			return retVal;
		}
	}

	retVal = sl_Connect(n->my_socket, (SlSockAddr_t *)&sAddr, addrSize);
	if (retVal < 0) {
		if (server_verify || retVal != -453) {
			sl_Close(n->my_socket);
			return retVal;
		}
	}

	SysTickIntRegister(SysTickIntHandler);
	SysTickPeriodSet(80000);
	SysTickEnable();

	return retVal;
}
#endif
