/* mqttnet.h
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

#ifndef WOLFMQTT_NET_H
#define WOLFMQTT_NET_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "settings.h"
#include "mqtt_socket.h"

/* For this limited implementation, only two members are required in the
 * Berkeley style sockaddr structure. */
struct sockaddr {
	const char *	hostname;
	uint16_t	remote_port;
	uint16_t	local_port;
};

#define SOCK_ADDR_IN    struct sockaddr
	
/* Setup defaults */
#ifndef CONTEXT_T
#define CONTEXT_T       uint8_t
#endif
#ifndef CONNECT_T
#define CONNECT_T       uint8_t
#endif
#ifndef CONTEXT_INVALID
#define CONTEXT_INVALID  ((CONTEXT_T)0)
#endif
#ifndef SOCK_CONNECT
#define SOCK_CONNECT    Gsm_OpenSocketCmd
#endif
#ifndef SOCK_SEND
#define SOCK_SEND(s, b, l, t) Gsm_SendDataCmd((s), (b), (uint16_t)(l), (uint32_t)t)
#endif
#ifndef SOCK_RECV
#define SOCK_RECV(b, l, t) Gsm_RecvRawData((b), (uint16_t)(l), (uint32_t)(t))
#endif
#ifndef SOCK_CLOSE
#define SOCK_CLOSE      Gsm_CloseSocketCmd
#endif
#ifndef SOCK_ADDR_IN
#define SOCK_ADDR_IN    struct sockaddr_in
#endif
#ifdef SOCK_ADDRINFO
#define SOCK_ADDRINFO   struct addrinfo
#endif
#ifndef DEFAULT_MQTT_CONTEXTID
#define DEFAULT_MQTT_CONTEXTID          1
#endif
#ifndef DEFAULT_MQTT_CONNECTID
#define DEFAULT_MQTT_CONNECTID          0
#endif
#ifndef SOCKET_DEFAULT_LOCAL_PORT
#define SOCKET_DEFAULT_LOCAL_PORT       0
#endif

/* Default MQTT host broker to use, when none is specified in the examples */
#define DEFAULT_MQTT_HOST       "iot.eclipse.org" /* broker.hivemq.com */

/* Local context for Net callbacks */
typedef enum {
	SOCK_BEGIN = 0,
	SOCK_CONN,
} NB_Stat;

typedef struct _SocketContext {
	CONTEXT_T	contextID;
	CONNECT_T	connectID;
	NB_Stat		stat;
	int		bytes;
	SOCK_ADDR_IN	addr;
} SocketContext;

/* Functions used to handle the MqttNet structure creation / destruction */
int MqttClientNet_Init(MqttNet* net);
int MqttClientNet_DeInit(MqttNet* net);


#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* WOLFMQTT_NET_H */
