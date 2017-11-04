/************************************************************************************************************
 * Copyright 2017 Rami Ojala / JAMK (K5643)                                                                 *
 *                                                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of                          *
 * this software and associated documentation files (the "Software"), to deal in the                        *
 * Software without restriction, including without limitation the rights to use, copy,                      *
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,                      *
 * and to permit persons to whom the Software is furnished to do so, subject to the                         *
 * following conditions:                                                                                    *
 *                                                                                                          *
 *  The above copyright notice and this permission notice shall be included                                 *
 *  in all copies or substantial portions of the Software.                                                  *
 *                                                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,                      *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A                            *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT                       *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION                        *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE                           *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                                   *
 *                                                                                                          *
 * https://opensource.org/licenses/MIT                                                                      *
 ************************************************************************************************************/

#ifndef MQTT_H
#define MQTT_H

#include "mqtt_adaptation.h"

#include <stdint.h>  // uint
#include <stddef.h>  // size_t
#include <stdbool.h> // bool

#define MQTT_MAX_MESSAGE_SIZE (0x80000000 - 1)

/**
 * @brief MQTT connection state
 *
 * Two major states, connected and disconnected.
 */
typedef enum MQTTState
{
    STATE_DISCONNECTED = 0,
    STATE_CONNECTED
} MQTTState_t;

/**
 * @brief MQTT action in mqtt() function.
 *
 * Defines action what mqtt() function shall do.
 */
typedef enum MQTTAction
{
    ACTION_DISCONNECT,
    ACTION_CONNECT,
    ACTION_PUBLISH,
    ACTION_SUBSCRIBE,
    ACTION_KEEPALIVE,
    ACTION_INIT,
    ACTION_PARSE_INPUT_STREAM
} MQTTAction_t;

/**
 * @brief MQTT message types
 *
 * @ref <a href="linkURLhttp://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">3.1.1 chapter 2.2.1 MQTT Control Packet type</a>
 */
typedef enum MQTTMessageType
{
    INVALIDCMD = 0,
    CONNECT    = 1,
    CONNACK,
    PUBLISH,
    PUBACK,
    PUBREC,
    PUBREL,
    PUBCOMP,
    SUBSCRIBE,
    SUBACK,
    UNSUBSCRIBE,
    UNSUBACK,
    PINGREQ,
    PINGRESP,
    DISCONNECT,
    MAXCMD
} MQTTMessageType_t;

/**
 * @brief MQTT status and error codes
 *
 * Value 0 is handled as successfull operation like in the specification.
 */
typedef enum MQTTErrorCodes
{
    InvalidArgument = -64,
    NoConnection,
    AllreadyConnected,
    PingNotSend,
    Successfull     = 0,
    InvalidVersion  = 1,
    InvalidIdentifier,
    ServerUnavailabe,
    BadUsernameOrPassword,
    NotAuthorized,
    PublishDecodeError
} MQTTErrorCodes_t;

/**
 * @brief MQTT message types
 *
 * @ref <a href="linkURLhttp://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">3.1.1 chapter 4.3 Quality of Service levels and protocol flows</a>
 */

typedef enum MQTTQoSLevel
{
    QoS0 = 0,
    QoS1,
    QoS2,
    QoSInvalid
} MQTTQoSLevel_t;


/****************************************************************************************
 * @section data structures                                                             *
 * Values and bitfields to fill MQTT messages correctly                                 *
 ****************************************************************************************/

#pragma pack(1)
typedef struct struct_flags_and_type
{
    uint8_t retain:1;      /* Retain or not                            */
    uint8_t dup:1;         /* one bit value, duplicate or not          */
    uint8_t qos:2;         /* Quality of service 0-2 @see MQTTQoSLevel */
    uint8_t message_type:4;/* @see MQTTMessageType                     */
} struct_flags_and_type_t;

/* FIXED HEADER */
typedef struct MQTT_fixed_header
{
    struct_flags_and_type_t flagsAndType;
    uint8_t                 length[4];
} MQTT_fixed_header_t;

/* VaARIABLE HEADER */
typedef struct MQTT_variable_header_connect_flags
{
    uint8_t reserved:1;
    uint8_t clean_session:1;
    uint8_t last_will:1;
    uint8_t last_will_qos:2;
    uint8_t permanent_will:1;
    uint8_t password:1;
    uint8_t username:1;
} MQTT_variable_header_connect_flags_t;

typedef struct MQTT_variable_header_connect
{
    uint8_t length[2];        /* In practise fixed to 4                    */
    uint8_t procol_name[4];   /* "MQTT" - string without null              */
    uint8_t protocol_version; /* Fixed to 3.1.1 = 0x04                     */
    uint8_t flags;            /* @see MQTT_variable_header_connect_flags_t */
    uint8_t keepalive[2];     /* Keepaive timer for the connection         */
} MQTT_variable_header_connect_t;

/* CONNECT */
typedef struct MQTT_connect
{
    MQTT_fixed_header_t                    fixed_header;
    MQTT_variable_header_connect_flags_t   connect_flags;
    uint16_t                               keepalive;
    uint8_t                              * last_will_topic;
    uint8_t                              * last_will_message;
    uint8_t                              * username;
    uint8_t                              * password;
    uint8_t                              * client_id;
} MQTT_connect_t;

/****************************************************************************************
 * @section state and data handling function pointers                                   *
 * Following function pointers are used with connection and subscribe functionalities.  *
 ****************************************************************************************/
typedef void (*connected_fptr_t)(MQTTErrorCodes_t a_status);

typedef void (*subscrbe_fptr_t)(MQTTErrorCodes_t   a_status,
                                uint8_t          * a_data_ptr,
                                uint32_t           a_data_len,
                                uint8_t          * a_topic_ptr,
                                uint16_t           a_topic_len);


/****************************************************************************************
 * @section input and output function pointers                                          *
 * Following function pointers are used to send and receive MQTT messages.              *
 * Must be implemnted.                                                                  *
 ****************************************************************************************/
typedef int (*data_stream_in_fptr_t)(uint8_t * a_data_ptr, size_t a_amount);

typedef int (*data_stream_out_fptr_t)(uint8_t * a_data_ptr, size_t a_amount);


/****************************************************************************************
 * @section shared data structure.                                                      *
 * MQTT stack uses this shared data sructure to keep its state and needed function      *
 * pointers in safe.                                                                    *
 ****************************************************************************************/
typedef struct MQTT_shared_data
{
    MQTTState_t              state;                   /* Connection state               */
    connected_fptr_t         connected_cb_fptr;       /* Connected callback             */
    subscrbe_fptr_t          subscribe_cb_fptr;       /* Subscribe callback             */
    uint8_t                * buffer;                  /* Pointer to transmit buffer     */
    size_t                   buffer_size;             /* Size of transmit buffer        */
    data_stream_out_fptr_t   out_fptr;                /* Sending out MQTT stream fptr   */
    uint32_t                 mqtt_packet_cntr;        /* MQTT packet indentifer counter */
    int32_t                  keepalive_in_ms;         /* Keepalive timer value          */
    int32_t                  time_to_next_ping_in_ms; /* Keepalive counter              */
    bool                     subscribe_status;        /* Internal subscribe status flag */
} MQTT_shared_data_t;

/****************************************************************************************
 * @section MQTT action structures                                                      *
 * MQTT action parameter structures for different actions.                              *
 ****************************************************************************************/
typedef struct MQTT_input_stream
{
    uint8_t  * data;
    uint32_t   size_of_data;
} MQTT_input_stream_t;

typedef struct MQTT_publish
{
    struct_flags_and_type_t   flags;
    uint8_t                 * topic_ptr;
    uint16_t                  topic_length;
    uint8_t                 * message_buffer_ptr;
    uint32_t                  message_buffer_size;
    uint8_t                 * output_buffer_ptr;
    uint32_t                  output_buffer_size;
} MQTT_publish_t;

typedef struct MQTT_subscribe
{
    MQTTQoSLevel_t   qos;
    uint8_t        * topic_ptr;
    uint16_t         topic_length;
} MQTT_subscribe_t;

typedef struct MQTT_action_data
{
    union {
        MQTT_shared_data_t  * shared_ptr;
        MQTT_connect_t      * connect_ptr;
        uint32_t              epalsed_time_in_ms;
        MQTT_input_stream_t * input_stream_ptr;
        MQTT_publish_t      * publish_ptr;
        MQTT_subscribe_t    * subscribe_ptr;
    } action_argument;
} MQTT_action_data_t;

/****************************************************************************************
 * @section Debug                                                                         *
 ****************************************************************************************/
/**
 * Debug hex print
 *
 * Dump out given data chunk in hex format
 *
 * @param a_data_ptr [in] pointer to beginning of the chunk.
 * @param a_size [in] size of the chunk in bytes.
 * @return None
 */
void hex_print(uint8_t * a_data_ptr, size_t a_size);


/****************************************************************************************
 * @section API                                                                         *
 ****************************************************************************************/
/**
 * mqtt core API
 *
 * API function to control, send and receive MQTT messges.
 * It uses data structures together with action parameter to request valid aciton with
 * proper parameters.
 *
 * @param a_action [in] action type see MQTTAction_t.
 * @param a_action_ptr [in] parameters for action see MQTT_action_data_t.
 * @return None
 */
MQTTErrorCodes_t mqtt(MQTTAction_t         a_action,
                      MQTT_action_data_t * a_action_ptr);

/**
 * mqtt_connect user API
 *
 * Initialize software stack and create connection to requested broker.
 *
 * @param a_client_name_ptr [in] name of client which is connecting to broker
 * @param a_keepalive_timeout [in] 0-x keepalive time in seconds (0=disabled).
 * @param a_username_str_ptr [in] username string (must be null ending).
 * @param a_password_str_ptr [in] password string (must be null ending).
 * @param a_last_will_topic_str_ptr [in] last will topic.
 * @param a_last_will_str_ptr [in] last will data string (must be null ending).
 * @param mqtt_shared_data_ptr [in] @see mqtt_shared_data_ptr.
 * @param a_output_buffer_ptr [in] common/shared output buffer.
 * @param a_output_buffer_size [in] maximum size of output buffer.
 * @param a_clean_session [in] is session clean or should broker restore it.
 * @param a_out_write_fptr [in] @see data_stream_out_fptr_t.
 * @param a_connected_fptr [in] @see connected_fptr_t.
 * @param a_subscribe_fptr [in] @see subscrbe_fptr_t.
 * @param a_timeout_in_sec [in] mqtt_connect timeout in seconds.
 * @return true if successfully connected.
 */
bool mqtt_connect(char                   * a_client_name_ptr,
                  uint16_t                 a_keepalive_timeout,
                  uint8_t                * a_username_str_ptr,
                  uint8_t                * a_password_str_ptr,
                  uint8_t                * a_last_will_topic_str_ptr,
                  uint8_t                * a_last_will_str_ptr,
                  MQTT_shared_data_t     * mqtt_shared_data_ptr,
                  uint8_t                * a_output_buffer_ptr,
                  size_t                   a_output_buffer_size,
                  bool                     a_clean_session,
                  data_stream_out_fptr_t   a_out_write_fptr,
                  connected_fptr_t         a_connected_fptr,
                  subscrbe_fptr_t          a_subscribe_fptr,
                  uint8_t                  a_timeout_in_sec);

/**
 * mqtt_disconnect user API
 *
 * Disconnect from server
 *
 * @return true when disconnected was successfully sent.
 */
bool mqtt_disconnect();

/**
 * mqtt_publish user API
 *
 * Publish data to given topic
 *
 * @param a_topic_ptr [in] topic (all values alloved = non chars).
 * @param a_topic_size [in] size of topic.
 * @param a_msg_ptr [in] pointer to data which shall be published.
 * @param a_msg_size [in] size of data to be published.
 * @return true when publish successfully formed and sent out.
 */
bool mqtt_publish(char * a_topic_ptr,
                  size_t a_topic_size,
                  char * a_msg_ptr,
                  size_t a_msg_size);
/**
 * mqtt_publish_buf user API
 *
 * Publish data to given topic, using dedicated transmit buffer.
 *
 * @param a_topic_ptr [in] topic (all values alloved = non chars).
 * @param a_topic_size [in] size of topic.
 * @param a_msg_ptr [in] pointer to data which shall be published.
 * @param a_msg_size [in] size of data to be published.
 * @param a_output_buffer_ptr [out] ponter to transmit buffer.
 * @param a_output_buffer_size [in] size of transmit buffer.
 * @return true when publish successfully formed and sent out.
 */
bool mqtt_publish_buf(char    * a_topic_ptr,
                      size_t    a_topic_size,
                      char    * a_msg_ptr,
                      size_t    a_msg_size,
                      uint8_t * a_output_buffer_ptr,
                      uint32_t  a_output_buffer_size);

/**
 * mqtt_subscribe user API
 *
 * Subscribe given topic. Wait SUBACK from broker before returns.
 *
 * @param a_topic [in] topic to be subscribed.
 * @param a_topic_size [in] size of topic.
 * @param a_timeout_in_sec [in] timeout in seconds
 * @return true when subscirbe succeeded.
 */
bool mqtt_subscribe(char    * a_topic,
                    uint16_t  a_topic_size,
                    uint8_t   a_timeout_in_sec);

/**
 * mqtt_keepalive user API
 *
 * Call reqularly to check if keepalive must be sent.
 * Function will send keepalive to broker based on
 * mqtt_connect keepalive parameters.
 *
 * @param a_topic [in] topic to be subscribed.
 * @param a_topic_size [in] size of topic.
 * @param a_timeout_in_sec [in] timeout in seconds
 * @return true when mqtt_keepalive succeeded.
 */
bool mqtt_keepalive(uint32_t a_duration_in_ms);

/**
 * mqtt_receive user API
 *
 * Feed received MQTT message to this function.
 * The function will parce it for you.
 *
 * @param a_data [in] beginning of received MQTT message.
 * @param a_amount [in] amount received data.
 * @return true when message successfully interpreted.
 */
bool mqtt_receive(uint8_t * a_data,
                  size_t    a_amount);

#endif /* MQTT_H */
