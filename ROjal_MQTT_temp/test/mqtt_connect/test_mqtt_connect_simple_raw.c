#include "mqtt.h"
#include "unity.h"
#include "../help/help.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

char buffer[1024*256];

void test_mqtt_socket()
{
    int socket_desc = open_mqtt_socket_();
    //Connect to remote server
    TEST_ASSERT_TRUE_MESSAGE(socket_desc >= 0, "MQTT Broker not running?");

    close_mqtt_socket_();
}

void test_mqtt_connect_simple_hack()
{
    // Connect to broker with socket
    int socket_desc = open_mqtt_socket_();
    TEST_ASSERT_TRUE_MESSAGE(socket_desc >= 0, "MQTT Broker not running?");

    uint8_t mqtt_raw_buffer[1024];
    memset(mqtt_raw_buffer, 0, sizeof(mqtt_raw_buffer));
    uint8_t sizeOfFixedHdr, sizeOfVarHdr;

    // Form fixed header for CONNECT msg. Message size is count to be 13.
    sizeOfFixedHdr = encode_fixed_header((MQTT_fixed_header_t*)&mqtt_raw_buffer, false, QoS0, false, CONNECT, 13);

    // Form variable header with MVP paramerters.
    sizeOfVarHdr = encode_variable_header_connect(&(mqtt_raw_buffer[sizeOfFixedHdr]), false, false, QoS0, false, false, false, 0);

    // Set device id into payload (size 1 and content o)
    mqtt_raw_buffer[sizeOfFixedHdr + sizeOfVarHdr + 1] = 1;
    mqtt_raw_buffer[sizeOfFixedHdr + sizeOfVarHdr + 2] = 'o';

    // Send CONNECT message to the broker
    TEST_ASSERT_FALSE_MESSAGE(send(socket_desc, mqtt_raw_buffer, sizeOfFixedHdr + sizeOfVarHdr + 3, 0) < 0, "Send failed");

    // Wait response from borker
    int rcv = recv(socket_desc, mqtt_raw_buffer , sizeof(mqtt_raw_buffer), 0);
    TEST_ASSERT_FALSE_MESSAGE(rcv  < 0, "Receive failed with error");
    TEST_ASSERT_FALSE_MESSAGE(rcv == 0, "No data received");

    bool              dup, retain;
    MQTTQoSLevel_t    qos = 2;
    MQTTMessageType_t type;
    uint32_t          size;
    // Decode fixed header
    uint8_t * nextHdr = decode_fixed_header(mqtt_raw_buffer, &dup, &qos, &retain, &type, &size);
    TEST_ASSERT_TRUE(NULL != nextHdr);
    TEST_ASSERT_TRUE(CONNACK == type);

    // Decode variable header
    uint8_t a_connection_state_ptr = 0;
    decode_variable_header_conack(nextHdr, &a_connection_state_ptr);
    TEST_ASSERT_EQUAL_MESSAGE(0, a_connection_state_ptr, "Connection attempt returned failure");

    // Form and send fixed header with DISCONNECT command ID
    sizeOfFixedHdr = encode_fixed_header((MQTT_fixed_header_t*)&mqtt_raw_buffer, false, QoS0, false, DISCONNECT, 0);
    TEST_ASSERT_FALSE_MESSAGE(send(socket_desc, mqtt_raw_buffer, sizeOfFixedHdr, 0) < 0, "Send failed");

    // Close socket
    close_mqtt_socket_();
}

void test_mqtt_connect_simple()
{
    // Connect to broker with socket
    int g_socket_desc = open_mqtt_socket_();
    TEST_ASSERT_TRUE_MESSAGE(g_socket_desc >= 0, "MQTT Broker not running?");

    uint8_t mqtt_raw_buffer[1024];
    MQTT_connect_t connect_params;

    uint8_t clientid[] = "JAMKtest test_mqtt_connect_simple";
    uint8_t aparam[]   = "\0";

    connect_params.client_id                    = clientid;
    connect_params.last_will_topic              = aparam;
    connect_params.last_will_message            = aparam;
    connect_params.username                     = aparam;
    connect_params.password                     = aparam;
    connect_params.keepalive                    = 0;
    connect_params.connect_flags.clean_session  = true;
    connect_params.connect_flags.permanent_will = false;
    connect_params.connect_flags.last_will_qos  = 0;
    MQTTErrorCodes_t ret = mqtt_connect_(mqtt_raw_buffer,
                                        sizeof(mqtt_raw_buffer),
                                        &data_stream_in_fptr_,
                                        &data_stream_out_fptr_,
                                        &connect_params,
                                        true);

    TEST_ASSERT_EQUAL_MESSAGE(0, ret, "MQTT Connect failed");

    // Form and send fixed header with DISCONNECT command ID
    uint8_t sizeOfFixedHdr = encode_fixed_header((MQTT_fixed_header_t*)&mqtt_raw_buffer, false, QoS0, false, DISCONNECT, 0);
    TEST_ASSERT_FALSE_MESSAGE(send(g_socket_desc, mqtt_raw_buffer, sizeOfFixedHdr, 0) < 0, "Send failed");

    // Close socket
    close_mqtt_socket_();
}


/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("MQTT connect simple");
    unsigned int tCntr = 1;

    /* CONNACK */
    RUN_TEST(test_mqtt_socket,                                  tCntr++);
    RUN_TEST(test_mqtt_connect_simple_hack,                     tCntr++);

    return (UnityEnd());
}
