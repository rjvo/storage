#include "mqtt.h"
#include "unity.h"
#include "../help/help.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

char buffer[1024*256];

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

void test_mqtt_connect_simple_username_and_password()
{
    // Connect to broker with socket
    int g_socket_desc = open_mqtt_socket_();
    TEST_ASSERT_TRUE_MESSAGE(g_socket_desc >= 0, "MQTT Broker not running?");

    uint8_t mqtt_raw_buffer[1024];
    MQTT_connect_t connect_params;

    uint8_t clientid[]  = "JAMKtest test_mqtt_connect_simple_username_and_password";
    uint8_t aparam[]    = "\0";
    uint8_t ausername[] = "aUsername";
    uint8_t apassword[] = "aPassword";

    connect_params.client_id                    = clientid;
    connect_params.last_will_topic              = aparam;
    connect_params.last_will_message            = aparam;
    connect_params.username                     = ausername;
    connect_params.password                     = apassword;
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

    TEST_ASSERT_FALSE_MESSAGE(ret != 0, "MQTT Connect failed");

    // Form and send fixed header with DISCONNECT command ID
    uint8_t sizeOfFixedHdr = encode_fixed_header((MQTT_fixed_header_t*)&mqtt_raw_buffer, false, QoS0, false, DISCONNECT, 0);
    TEST_ASSERT_FALSE_MESSAGE(send(g_socket_desc, mqtt_raw_buffer, sizeOfFixedHdr, 0) < 0, "Send failed");

    // Close socket
    close_mqtt_socket_();
}

void test_mqtt_connect_simple_all_details()
{
    // Connect to broker with socket
    int g_socket_desc = open_mqtt_socket_();
    TEST_ASSERT_TRUE_MESSAGE(g_socket_desc >= 0, "MQTT Broker not running?");

    uint8_t mqtt_raw_buffer[1024];
    MQTT_connect_t connect_params;

    uint8_t clientid[]  = "JAMKtest test_mqtt_connect_simple_all_details";

    uint8_t ausername[] = "aUsername";
    uint8_t apassword[] = "aPassword";
    uint8_t alwt[]      = "/IoT/device/state";
    uint8_t alwm[]      = "Offline";

    connect_params.client_id         = clientid;
    connect_params.last_will_topic   = alwt;
    connect_params.last_will_message = alwm;
    connect_params.username          = ausername;
    connect_params.password          = apassword;

    connect_params.keepalive                    = 60;
    connect_params.connect_flags.clean_session  = true;
    connect_params.connect_flags.permanent_will = false;
    connect_params.connect_flags.last_will_qos  = 0;
    MQTTErrorCodes_t ret = mqtt_connect_(mqtt_raw_buffer,
                                        sizeof(mqtt_raw_buffer),
                                        &data_stream_in_fptr_,
                                        &data_stream_out_fptr_,
                                        &connect_params,
                                        true);

    TEST_ASSERT_FALSE_MESSAGE(ret != 0, "MQTT Connect failed");

    // MQTT disconnect
    TEST_ASSERT_TRUE(mqtt_disconnect(&data_stream_out_fptr_) == Successfull);

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
    RUN_TEST(test_mqtt_connect_simple,                          tCntr++);
    RUN_TEST(test_mqtt_connect_simple_username_and_password,    tCntr++);
    RUN_TEST(test_mqtt_connect_simple_all_details,              tCntr++);
    return (UnityEnd());
}
