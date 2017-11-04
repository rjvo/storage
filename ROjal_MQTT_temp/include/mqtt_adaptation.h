#ifndef MQTT_ADAPTATION_H
#define MQTT_ADAPTATION_H

#ifdef BUILD_DEFAULT_C_LIBS
/*******************************************************************************************************************
 * Linux build env Linux build env Linux build env Linux build env Linux build env Linux build env Linux build env *
 *******************************************************************************************************************/

#include <unistd.h>  // sleep
#include <stdio.h>   // printf
#include <string.h>  // memcpy, strlen

/**
 * mqtt_printf
 *
 * printf function to print out debugging data.
 *
 */
#define mqtt_printf printf

/**
 * mqtt_memcpy
 *
 * memcpy function to copy data from source to destination.
 *
 */
#define mqtt_memcpy memcpy

/**
 * mqtt_memset
 *
 * Set given memory with specified value = memset.
 *
 */
#define mqtt_memset memset

#define mqtt_sleep sleep

#define mqtt_strlen strlen

#endif /* BUILD_DEFAULT_C_LIBS */

#ifdef BUILD_FREERTOS
/*******************************************************************************************************************
 *   FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS FreeRTOS   *
 *******************************************************************************************************************/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memcpy, strlen

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "FreeRTOSIPConfig.h"

#define DEBUG

#define FREERTOS_MQTT_PORT 1883

#if 0
/* test.mosquitto.org IP address */
#define FREERTOS_MQTT_ADDR1 37
#define FREERTOS_MQTT_ADDR2 187
#define FREERTOS_MQTT_ADDR3 106
#define FREERTOS_MQTT_ADDR4 16
#else
#define FREERTOS_MQTT_ADDR1 192
#define FREERTOS_MQTT_ADDR2 168
#define FREERTOS_MQTT_ADDR3 0
#define FREERTOS_MQTT_ADDR4 201
#endif

#define FREERTOS_CLIENT_ID "FreeRTOS_ROjal_MQTT_Client"

#define FREERTOS_MAX_MQTT_SIZE 1024

/**
 * mqtt_printf
 *
 * printf function to print out debugging data.
 *
 */
#define mqtt_printf vLoggingPrintf

/**
 * mqtt_memcpy
 *
 * memcpy function to copy data from source to destination.
 *
 */
#define mqtt_memcpy memcpy

/**
 * mqtt_memset
 *
 * Set given memory with specified value = memset.
 *
 */
#define mqtt_memset memset


#define mqtt_sleep(x) vTaskDelay(x/portTICK_PERIOD_MS)

#define mqtt_strlen strlen

#endif /* BUILD_FREERTOS */

#endif
