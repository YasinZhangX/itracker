#include "task_queue_timer.h"

//mqtt task,queue,timer
xTaskHandle xMqttTaskHandle;
xTaskHandle xMqttRcvTaskHandle;

xQueueHandle xQueueMqtt;
xTimerHandle xMqttTimer;

xTaskHandle mqtt_messageTaskHandle;
//xSemaphoreHandle xBinarySemaphore;

//uart task,queue,timer
xTaskHandle xUartTaskHandle;
xTaskHandle xUartTaskHandle1;
xQueueHandle xQueueUart;
xSemaphoreHandle xSemaphore;
xSemaphoreHandle xSemaphoremqttmsg;

//wifi task,queue,timer
xTaskHandle xWifiTaskHandle;
xQueueHandle xQueueWifi;
xQueueHandle xQueueSmartSer;
xTimerHandle xTcpSendTimer;
xSemaphoreHandle xWifiAlive;

//key and led timer
xTimerHandle xLedFastTimer;
xTimerHandle xLedSlowTimer;
xTimerHandle xLedToggleTimer;
xTimerHandle xKeyTimer;
