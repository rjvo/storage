#ifndef ASLEEP_H
#define ASLEEP_H

#include <stdint.h>  // uint
#include <stddef.h>  // size_t
#include <stdbool.h> // bool

#include "mqtt.h"

void asleep(int milliseconds);

void close_mqtt_socket_();

int open_mqtt_socket_();

int data_stream_out_fptr_(uint8_t * a_data_ptr, size_t a_amount);

int data_stream_in_fptr_(uint8_t * a_data_ptr, size_t a_amount);

void subscrbe_cb_(MQTTErrorCodes_t a_status,
                 uint8_t * a_data_ptr,
                 uint32_t a_data_len,
                 uint8_t * a_topic_ptr,
                 uint16_t a_topic_len);

void connected_cb_(MQTTErrorCodes_t a_status);

/* Functions not declared in mqtt.h - internal functions */
extern uint8_t encode_fixed_header(MQTT_fixed_header_t * output,
                                   bool dup,
                                   MQTTQoSLevel_t qos,
                                   bool retain,
                                   MQTTMessageType_t messageType,
                                   uint32_t msgSize);

extern uint8_t * decode_fixed_header(uint8_t * a_input_ptr,
                                     bool * a_dup_ptr,
                                     MQTTQoSLevel_t * a_qos_ptr,
                                     bool * a_retain_ptr,
                                     MQTTMessageType_t * a_message_type_ptr,
                                     uint32_t * a_message_size_ptr);

extern uint8_t encode_variable_header_connect(uint8_t * a_output_ptr,
                                              bool a_clean_session,
                                              bool a_last_will,
                                              MQTTQoSLevel_t a_last_will_qos,
                                              bool a_permanent_last_will,
                                              bool a_password,
                                              bool a_username,
                                              uint16_t a_keepalive);

extern uint8_t * decode_variable_header_conack(uint8_t * a_input_ptr, uint8_t * a_connection_state_ptr);

extern MQTTErrorCodes_t mqtt_connect_(uint8_t * a_message_buffer_ptr,
                                     size_t a_max_buffer_size,
                                     data_stream_in_fptr_t a_in_fptr,
                                     data_stream_out_fptr_t a_out_fptr,
                                     MQTT_connect_t * a_connect_ptr,
                                     bool wait_and_parse_response);

extern MQTTErrorCodes_t mqtt_disconnect_(data_stream_out_fptr_t a_out_fptr);

extern MQTTErrorCodes_t mqtt_ping_req(data_stream_out_fptr_t a_out_fptr);

extern MQTTErrorCodes_t mqtt_parse_ping_ack(uint8_t * a_message_in_ptr);


#endif