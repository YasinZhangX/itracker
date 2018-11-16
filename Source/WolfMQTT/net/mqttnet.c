/* mqttnet.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
 *
 * This file is part of wolfMQTT.
 *
 * wolfMQTT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfMQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* Include the autoconf generated config.h */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mqtt_client.h"
#include "mqttnet.h"
#include "mqttexample.h"


#include "rak_uart_gsm.h"

/* FreeRTOS TCP */
#ifdef FREERTOS_TCP
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_DNS.h"
#include "FreeRTOS_Sockets.h"

#define SOCKET_T                     Socket_t
#define SOCK_ADDR_IN                 struct freertos_sockaddr

/* FreeRTOS and LWIP */
#elif defined(FREERTOS)
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Windows */
#elif defined(USE_WINDOWS_API)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#define SOCKET_T SOCKET
#define SOERROR_T       char
#define SELECT_FD(fd)   (fd)
#define SOCKET_INVALID  ((SOCKET_T)INVALID_SOCKET)
#define SOCK_CLOSE      closesocket
#define SOCK_SEND(s, b, l, f) send((s), (const char *)(b), (size_t)(l), (f))
#define SOCK_RECV(s, b, l, f) recv((s), (char *)(b), (size_t)(l), (f))

/* Freescale MQX / RTCS */
#elif defined(FREESCALE_MQX) || defined(FREESCALE_KSDK_MQX)
#if defined(FREESCALE_MQX)
#include <posix.h>
#endif
#include <rtcs.h>
/* Note: Use "RTCS_geterror(sock->fd);" to get error number */

/* Microchip MPLABX Harmony, TCP/IP */
#elif defined(MICROCHIP_MPLAB_HARMONY)
#include "app.h"
#include "mqtt_client.h"
#include "mqttnet.h"

#include "system_config.h"
#include "tcpip/tcpip.h"
#include <sys/errno.h>
#include <errno.h>
struct timeval {
	int	tv_sec;
	int	tv_usec;
};

#define SOCKET_INVALID (-1)
#define SO_ERROR 0
#define SOERROR_T uint8_t
#undef  FD_ISSET
#define FD_ISSET(f1, f2) (1 == 1)
#define SOCK_CLOSE      closesocket

/* Linux */
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#endif

/* Include the example code */
#include "mqttexample.h"

/* Private functions */
static int NetConnect(void *context, const char *host, word16 port, int timeout_ms)
{
	SocketContext *sock = (SocketContext *)context;
	int rc = -1;

	switch (sock->stat) {
	case SOCK_BEGIN:
		sock->addr.hostname = host;
		sock->addr.remote_port = port;
		sock->addr.local_port = SOCKET_DEFAULT_LOCAL_PORT;  // default local_port is 0

		/* Create socket */
		sock->contextID = DEFAULT_MQTT_CONTEXTID;
		sock->connectID = DEFAULT_MQTT_CONNECTID;

		if (sock->contextID == 0) {
			DPRINTF(LOG_ERROR, "sock->contextID invalid.\r\n");
			break;
		}

		(void)timeout_ms;

		sock->stat = SOCK_CONN;

		FALL_THROUGH;
	case SOCK_CONN:
		/* Start connect */
		rc = SOCK_CONNECT(context, GSM_TCP_TYPE, GSM_BUFFER_ACCESS_MODE);
		break;
	}

	return rc;
}

static int NetRead(void *context, byte *buf, int buf_len, int timeout_ms)
{
	SocketContext *sock = (SocketContext *)context;
	int rc = -1, timeout = 0;
	int lenToBeRead = 0;
	int retry_count = 4;

	if (context == NULL || buf == NULL || buf_len <= 0)
		return MQTT_CODE_ERROR_BAD_ARG;

	sock->bytes = 0;

	/* Loop until buf_len has been read, error or timeout */
	while ((sock->bytes < buf_len) && (timeout == 0) && (retry_count-- > 0)) {
		/* Wait for any event within the socket set. */
		lenToBeRead = Gsm_ReadRecvBufferCmd(context, buf_len - sock->bytes, timeout_ms);
		if (lenToBeRead != -1) {
			/* Try and read number of lenToBeRead provided,
			 *  minus what's already been read */
			rc = (int)SOCK_RECV(&buf[sock->bytes], lenToBeRead, timeout_ms);

			if (rc <= 0) {
				timeout = 1;    /* timeout */
				break;
			} else {
				sock->bytes += rc;      /* Data */
			}
		} else {
			timeout = 1;
		}
	}

	if (rc == 0 || timeout) {
		rc = MQTT_CODE_ERROR_TIMEOUT;
	} else if (rc < 0) {
		PRINTF("NetRead: Error %d", rc);
		rc = MQTT_CODE_ERROR_NETWORK;
	} else {
		rc = sock->bytes;
	}
	sock->bytes = 0;

	return rc;
}

static int NetWrite(void *context, const byte *buf, int buf_len, int timeout_ms)
{
	SocketContext *sock = (SocketContext *)context;
	int rc = -1;

	(void)timeout_ms;

	if (context == NULL || buf == NULL || buf_len <= 0)
		return MQTT_CODE_ERROR_BAD_ARG;

	rc = (int)SOCK_SEND(sock, (byte*)buf, buf_len, timeout_ms);

	if (rc < 0) {
#ifdef WOLFMQTT_NONBLOCK
		if (rc == -pdFREERTOS_ERRNO_EWOULDBLOCK)
			return MQTT_CODE_CONTINUE;

#endif
		PRINTF("NetWrite: Error %d", rc);
		rc = MQTT_CODE_ERROR_NETWORK;
	}

	return rc;
}

static int NetDisconnect(void *context)
{
	SocketContext *sock = (SocketContext *)context;

	if (sock) {
		SOCK_CLOSE(sock);
		sock->stat = SOCK_BEGIN;
		sock->bytes = 0;
	}

	return 0;
}



/* Public Functions */
int MqttClientNet_Init(MqttNet *net)
{
#if defined(USE_WINDOWS_API) && !defined(FREERTOS_TCP)
	WSADATA wsd;
	WSAStartup(0x0002, &wsd);
#endif

#ifdef MICROCHIP_MPLAB_HARMONY
	static IPV4_ADDR dwLastIP[2] = { { -1 }, { -1 } };
	IPV4_ADDR ipAddr;
	int Dummy;
	int nNets;
	int i;
	SYS_STATUS stat;
	TCPIP_NET_HANDLE netH;

	stat = TCPIP_STACK_Status(sysObj.tcpip);
	if (stat < 0)
		return MQTT_CODE_CONTINUE;

	nNets = TCPIP_STACK_NumberOfNetworksGet();
	for (i = 0; i < nNets; i++) {
		netH = TCPIP_STACK_IndexToNet(i);
		ipAddr.Val = TCPIP_STACK_NetAddress(netH);
		if (ipAddr.v[0] == 0)
			return MQTT_CODE_CONTINUE;
		if (dwLastIP[i].Val != ipAddr.Val) {
			dwLastIP[i].Val = ipAddr.Val;
			PRINTF("%s", TCPIP_STACK_NetNameGet(netH));
			PRINTF(" IP Address: ");
			PRINTF("%d.%d.%d.%d\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
		}
	}
#endif  /* MICROCHIP_MPLAB_HARMONY */

	if (net) {
		XMEMSET(net, 0, sizeof(MqttNet));
		net->connect = NetConnect;
		net->read = NetRead;
		net->write = NetWrite;
		net->disconnect = NetDisconnect;
		net->context = (SocketContext *)WOLFMQTT_MALLOC(sizeof(SocketContext));
		if (net->context == NULL)
			return MQTT_CODE_ERROR_MEMORY;
		XMEMSET(net->context, 0, sizeof(SocketContext));

		((SocketContext *)(net->context))->stat = SOCK_BEGIN;
	}

	return MQTT_CODE_SUCCESS;
}

int MqttClientNet_DeInit(MqttNet *net)
{
	if (net) {
		if (net->context)
			WOLFMQTT_FREE(net->context);
		XMEMSET(net, 0, sizeof(MqttNet));
	}
	return 0;
}
