/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * A set of Tx and a set of Rx tasks are created.  The Tx tasks send TCP echo
 * requests to the standard echo port (port 7) on the IP address set by the
 * configECHO_SERVER_ADDR0 to configECHO_SERVER_ADDR3 constants.  The Rx tasks
 * then use the same socket to receive and validate the echoed reply.
 *
 * See the following web page for essential demo usage and configuration
 * details:
 * http://www.FreeRTOS.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/examples_FreeRTOS_simulator.html
 */

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

/* MQTT */
#include "mqtt.h"

/* Exclude the whole file if FreeRTOSIPConfig.h is configured to use UDP only. */
#if( ipconfigUSE_TCP == 1 )

/* Short delay used between demo cycles to ensure the network does not get too
congested. */
#define echoLOOP_DELAY	pdMS_TO_TICKS( 500UL )

/* The echo server is assumed to be on port 7, which is the standard echo
protocol port. */
#define echoECHO_PORT	( 1883 )

/* Dimensions the buffer used by prvEchoClientTxTask() to send a multiple of
MSS bytes at once. */
#define echoLARGE_BUFFER_SIZE_MULTIPLIER ( 10 )


/*-----------------------------------------------------------*/

static void prvConnectTask( void *pvParameters );
static void prvReceiveTask( void *pvParameters );
static void prvAliveTask(void *pvParameters);
static void prvPublishTask(void *pvParameters);

/* Rx and Tx time outs are used to ensure the sockets do not wait too long for
missing data. */
static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS( 500 );
static const TickType_t xSendTimeOut = pdMS_TO_TICKS( 2000 );

/* Counters for each task created - for inspection only. */
static uint32_t ulTxTaskCycles = 0,	ulRxTaskCycles = 0;

/* The queue used by prvEchoClientTxTask() to send the next socket to use to
prvEchoClientRxTask(). */
static QueueHandle_t xSocketPassingQueue = NULL;

/* The event group used by the prvEchoClientTxTask() and prvEchoClientRxTask()
to synchronise prior to commencing a cycle using a new socket. */
static EventGroupHandle_t xSyncEventGroup = NULL;

/* Flag used to inform the Rx task that the socket is about to be shut down. */
int32_t lShuttingDown = pdFALSE;

static MQTT_shared_data_t mqtt_shared_data;
static uint8_t a_output_buffer[1024]; /* Shared buffer */
static Socket_t xSocket = FREERTOS_INVALID_SOCKET;

static const uint32_t gKeepAliveTime = 60000; // in milliseconds

/*-----------------------------------------------------------*/

void vStartTCPEchoClientTasks_SeparateTasks( uint16_t usTaskStackSize, UBaseType_t uxTaskPriority )
{
	/* Create the queue used to pass the socket to use from the Tx task to the
	Rx task. */
	xSocketPassingQueue = xQueueCreate( 1, sizeof( Socket_t ) );
	configASSERT( xSocketPassingQueue );

	/* Create the event group used by the Tx and Rx tasks to synchronise prior
	to commencing a cycle using a new socket. */
	xSyncEventGroup = xEventGroupCreate();
	configASSERT( xSyncEventGroup );

	/* Create the task that sends to an echo server, but lets a different task
	receive the reply on the same socket. */
	xTaskCreate( 	prvConnectTask,			/* The function that implements the task. */
					"Connect",				/* Just a text name for the task to aid debugging. */
					usTaskStackSize,		/* The stack size is defined in FreeRTOSIPConfig.h. */
					NULL,					/* The task parameter, not used in this case. */
					uxTaskPriority,			/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
					NULL );					/* The task handle is not used. */

	/* Create the task that receives the reply to echos initiated by the
	prvEchoClientTxTask() task. */
	xTaskCreate(prvReceiveTask,  "Receive", configMINIMAL_STACK_SIZE, NULL, uxTaskPriority + 1, NULL );
	xTaskCreate(prvAliveTask,    "Alive",   configMINIMAL_STACK_SIZE, NULL, uxTaskPriority + 1, NULL);
	xTaskCreate(prvPublishTask,  "Publish", configMINIMAL_STACK_SIZE, NULL, uxTaskPriority + 1, NULL);
}
/*-----------------------------------------------------------*/

void connected_cb(MQTTErrorCodes_t a_status)
{
	if (Successfull == a_status) {
		FreeRTOS_printf(("Connected CB SUCCESSFULL\n"));
		mqtt_subscribe("RMQTTin", 7, 5);
	} else {
		FreeRTOS_printf(("Connected CB FAIL %i\n", a_status));
	}
}

void subscrbe_cb(MQTTErrorCodes_t a_status,
			 	 uint8_t * a_data_ptr,
			 	 uint32_t a_data_len,
				 uint8_t * a_topic_ptr,
				 uint16_t a_topic_len)
{
	if (Successfull == a_status) {
		FreeRTOS_printf(("Subscribed CB SUCCESSFULL\n"));
		for (uint16_t i = 0; i < a_topic_len; i++)
			printf("%c", a_topic_ptr[i]);
		printf(": ");
		for (uint32_t i = 0; i < a_data_len; i++)
			printf("%c", a_data_ptr[i]);
		printf("\n");
	}
	else {
		FreeRTOS_printf(("Subscribed CB FAIL %i\n", a_status));
	}
}


void socket_write(uint8_t * a_data_ptr, size_t a_amount)
{

	int lTransmitted = 0;
	int lReturned = 0;

	/* Keep sending until the entire buffer has been sent. */
	while (lTransmitted < a_amount)
	{
		/* How many bytes are left to send?  Attempt to send them
		all at once (so the length is potentially greater than the
		MSS). */
		size_t xLenToSend = a_amount - lTransmitted;

		lReturned = (int) FreeRTOS_send(xSocket,	 /* The socket being sent to. */
								          (void *) a_data_ptr,	/* The data being sent. */
										  xLenToSend, /* The length of the data being sent. */
										  0);		 /* ulFlags. */

		if (lReturned >= 0)
		{
			/* Data was sent successfully. */
			lTransmitted += lReturned;
		}
		else
		{
			/* Error - close the socket. */
			FreeRTOS_printf(("Socket off\r\n"));
			xSocket = FREERTOS_INVALID_SOCKET;
			lShuttingDown = pdTRUE;
			break;
		}
	}

	if (lReturned < 0)
	{
		/* The data was not sent for some reason - close the
		socket. */
		FreeRTOS_printf(("Socket error\r\n"));
		lShuttingDown = pdTRUE;
		xSocket = FREERTOS_INVALID_SOCKET;
	}
}

static void prvConnectTask(void *pvParameters)
{
	struct freertos_sockaddr xEchoServerAddress;
	BaseType_t lReturned = 0;
	WinProperties_t xWinProps;

	/* Avoid warning about unused parameter. */
	(void)pvParameters;

	/* Fill in the required buffer and window sizes. */
	xWinProps.lTxBufSize = 6 * ipconfigTCP_MSS;
	xWinProps.lTxWinSize = 3;
	xWinProps.lRxBufSize = 6 * ipconfigTCP_MSS;
	xWinProps.lRxWinSize = 3;
	
	/* Echo requests are sent to the echo server.  The address of the echo
	server is configured by the constants configECHO_SERVER_ADDR0 to
	configECHO_SERVER_ADDR3 in FreeRTOSConfig.h. */
	xEchoServerAddress.sin_port = FreeRTOS_htons(FREERTOS_MQTT_PORT);
	xEchoServerAddress.sin_addr = FreeRTOS_inet_addr_quick(FREERTOS_MQTT_ADDR1,
														   FREERTOS_MQTT_ADDR2,
														   FREERTOS_MQTT_ADDR3,
														   FREERTOS_MQTT_ADDR4);

		for (;;)
		{
			lShuttingDown = pdFALSE;

			/* Create a socket. */
			xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
			configASSERT(xSocket != FREERTOS_INVALID_SOCKET);

			/* Set a time out so a missing reply does not cause the task to block indefinitely. */
			FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof(xReceiveTimeOut));
			FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof(xSendTimeOut));

			/* Set the buffer and window sizes. */
			FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_WIN_PROPERTIES, (void *)&xWinProps, sizeof(xWinProps));

			/* Attempt to connect to the echo server. */
			if (FreeRTOS_connect(xSocket, &xEchoServerAddress, sizeof(xEchoServerAddress)) == 0)
			{
				FreeRTOS_printf(("Socket on\r\n"));
				/* Send the connected socket to the other tasks to start them. */
				lReturned = xQueueSend(xSocketPassingQueue, &xSocket, portMAX_DELAY);
				configASSERT(lReturned == pdPASS);
				lReturned = xQueueSend(xSocketPassingQueue, &xSocket, portMAX_DELAY);
				configASSERT(lReturned == pdPASS);
				lReturned = xQueueSend(xSocketPassingQueue, &xSocket, portMAX_DELAY);
				configASSERT(lReturned == pdPASS);

				if (mqtt_connect(FREERTOS_CLIENT_ID,
								gKeepAliveTime / 1000, // in seconds
								"",
								"",
								"",
								"",
								&mqtt_shared_data,
								a_output_buffer,
								sizeof(a_output_buffer),
								1,
								&socket_write,
								&connected_cb,
								&subscrbe_cb,
								10)) {

					mqtt_publish("state",
								  5,
								  "online",
								  7);

					while (xSocket!= FREERTOS_INVALID_SOCKET) {
						vTaskDelay(100 / portTICK_PERIOD_MS);
					}
				}
				else {
					vTaskDelay(1000 / portTICK_PERIOD_MS);
					FreeRTOS_printf(("MQTT failed\r\n"));
				}
			}

			xSocket = FREERTOS_INVALID_SOCKET;

			/* Inform the other task that is using the same socket that this
			task is waiting to shut the socket. */
			lShuttingDown = pdTRUE;

			/* Initiate graceful shut down. */
			FreeRTOS_shutdown(xSocket, FREERTOS_SHUT_RDWR);

			/* The Rx task is no longer using the socket so the socket can be
			closed. */
			FreeRTOS_closesocket(xSocket);

			vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}
/*-----------------------------------------------------------*/

int32_t get_remainingsize(uint8_t * a_input_ptr)
{
	uint32_t multiplier = 1;
	uint32_t value = 0;
	uint8_t  cnt = 1;
	uint8_t  aByte = 0;

	/* Verify input parameters */
	if (NULL == a_input_ptr)
		return -1;

	do {
		aByte = a_input_ptr[cnt++];
		value += (aByte & 127) * multiplier;
		if (multiplier > (128 * 128 * 128))
			return -2;

		multiplier *= 128;
	} while ((aByte & 128) != 0);
	value += (cnt); //Add count header to the overal size of MQTT packet to be received
	return value;
}

static void prvReceiveTask( void *pvParameters )
{
BaseType_t lReceived, lReturned = 0;
Socket_t xSocketTmp = FREERTOS_INVALID_SOCKET;
	( void ) pvParameters;

	for( ;; )
	{
		/* Wait to receive the socket that will be used from the Tx task. */
		xQueueReceive( xSocketPassingQueue, &xSocketTmp, portMAX_DELAY );

		while ((xSocketTmp != FREERTOS_INVALID_SOCKET) &&
				(pdFALSE == lShuttingDown)) {

			char header[32] = { 0 };
			int bytes_read = FreeRTOS_recv((xSocketTmp), header, sizeof(header) - 1, 0);

			if (2 <= bytes_read) {
				uint32_t remaining_bytes = get_remainingsize((uint8_t*)header) - bytes_read;

				if (0 < remaining_bytes && FREERTOS_MAX_MQTT_SIZE > remaining_bytes) {
					/* Need to read more data */
					char * buff = (char*)pvPortMalloc(remaining_bytes + bytes_read + 128 /* safety buffer*/);
					memcpy(buff, header, bytes_read);
					while (remaining_bytes) {
						int nxt_bytes_read = FreeRTOS_recv(xSocketTmp, &buff[bytes_read], remaining_bytes, 0);
						remaining_bytes -= nxt_bytes_read;
						bytes_read += nxt_bytes_read;
					}
					mqtt_receive((uint8_t*)buff, (uint32_t)bytes_read);
					vPortFree(buff);
				}
				else {
					/* Send packets dirctly to the MQTT client, which fit into 32B buffer */
					mqtt_receive((uint8_t*)header, (uint32_t)bytes_read);
				}
			} else if (0 > bytes_read) {
				xSocket = FREERTOS_INVALID_SOCKET;
				lShuttingDown = pdTRUE;
			}
		} // Loop forewer
	}
}


static void prvAliveTask(void *pvParameters)
{
	Socket_t xSocketTmp = FREERTOS_INVALID_SOCKET;
	(void)pvParameters;

	for (;; )
	{
		/* Wait to receive the socket that will be used from the Tx task. */
		xQueueReceive(xSocketPassingQueue, &xSocketTmp, portMAX_DELAY);

		if (0 < gKeepAliveTime) {
			for (;; ) {
				vTaskDelay((gKeepAliveTime/2) / portTICK_PERIOD_MS); // Divided by 2, because of windows inaccurate ticks
				FreeRTOS_printf(("MQTT WD Feed\r\n"));
				if (false == mqtt_keepalive(gKeepAliveTime / 4 * 3)) {
					xSocketTmp = FREERTOS_INVALID_SOCKET;
					lShuttingDown = pdTRUE;
					break;
				}
			}
		}
	}
}

static void prvPublishTask(void *pvParameters)
{
	Socket_t xSocketTmp = FREERTOS_INVALID_SOCKET;
	(void)pvParameters;

	for (;; )
	{
		/* Wait to receive the socket that will be used from the Tx task. */
		xQueueReceive(xSocketPassingQueue, &xSocketTmp, portMAX_DELAY);
		const char topic[] = "/lampotila/alakerta";
		for (;;) {
			uint32_t cntr = xTaskGetTickCount();
			vTaskDelay( 1000 / portTICK_PERIOD_MS); // Divided by 2, because of windows inaccurate ticks
			FreeRTOS_printf(("MQTT Publish Lampotila\r\n"));

			if (false == mqtt_publish(topic,
									  sizeof(topic) - 1, // Do not count null into the length
									  &cntr,
									  sizeof(cntr))){
				xSocketTmp = FREERTOS_INVALID_SOCKET;
				lShuttingDown = pdTRUE;
				break;
			}
		}
	}
}

#endif /* ipconfigUSE_TCP */

