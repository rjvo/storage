#include "mqtt.h"
#include "unity.h"
#include "../help/help.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

char buffer[1024*256];

void test_mqtt_connect_simple_keepalive()
{
    // Connect to broker with socket
    int g_socket_desc = open_mqtt_socket_();
    TEST_ASSERT_TRUE_MESSAGE(g_socket_desc >= 0, "MQTT Broker not running?");

    uint8_t mqtt_raw_buffer[1024];
    MQTT_connect_t connect_params;

    uint8_t clientid[] = "JAMKtest test_mqtt_connect_simple_keepalive";
    uint8_t aparam[]   = "\0";

    connect_params.client_id                    = clientid;
    connect_params.last_will_topic              = aparam;
    connect_params.last_will_message            = aparam;
    connect_params.username                     = aparam;
    connect_params.password                     = aparam;
    connect_params.keepalive                    = 2;
    connect_params.connect_flags.clean_session  = true;
    connect_params.connect_flags.last_will_qos  = 0;
    connect_params.connect_flags.permanent_will = false;

    MQTTErrorCodes_t ret = mqtt_connect_(mqtt_raw_buffer,
                                        sizeof(mqtt_raw_buffer),
                                        &data_stream_in_fptr_,
                                        &data_stream_out_fptr_,
                                        &connect_params,
                                        true);

    TEST_ASSERT_FALSE_MESSAGE(ret != 0, "MQTT Connect failed");

    for (uint8_t i=0; i<1; i++) {
        uint8_t mqtt_raw_buffer[64];
        asleep(1000);
        // MQTT PING REQ
        TEST_ASSERT_TRUE(mqtt_ping_req(&data_stream_out_fptr_) == Successfull);

        // Wait PINGRESP from borker
        int rcv = recv(g_socket_desc, mqtt_raw_buffer , sizeof(mqtt_raw_buffer) , 0);

        TEST_ASSERT_FALSE_MESSAGE(rcv < 0,  "Receive failed with error");
        TEST_ASSERT_FALSE_MESSAGE(rcv == 0, "No data received");
        TEST_ASSERT_EQUAL(Successfull, mqtt_parse_ping_ack(mqtt_raw_buffer));
    }

    // MQTT disconnect
    TEST_ASSERT_TRUE(mqtt_disconnect(&data_stream_out_fptr_) == Successfull);

    // Close socket
    close_mqtt_socket_();
}


void test_mqtt_connect_simple_keepalive_timeout()
{
    // Connect to broker with socket
    int g_socket_desc = open_mqtt_socket_();
    TEST_ASSERT_TRUE_MESSAGE(g_socket_desc >= 0, "MQTT Broker not running?");

    uint8_t mqtt_raw_buffer[1024];
    MQTT_connect_t connect_params;

    uint8_t clientid[] = "JAMKtest test_mqtt_connect_simple_keepalive_timeout";
    uint8_t aparam[]   = "\0";

    connect_params.client_id                    = clientid;
    connect_params.last_will_topic              = aparam;
    connect_params.last_will_message            = aparam;
    connect_params.username                     = aparam;
    connect_params.password                     = aparam;
    connect_params.keepalive                    = 2;
    connect_params.connect_flags.clean_session  = true;
    connect_params.connect_flags.last_will_qos  = 0;
    connect_params.connect_flags.permanent_will = false;

    MQTTErrorCodes_t ret = mqtt_connect_(mqtt_raw_buffer,
                                        sizeof(mqtt_raw_buffer),
                                        &data_stream_in_fptr_,
                                        &data_stream_out_fptr_,
                                        &connect_params,
                                        true);

    TEST_ASSERT_FALSE_MESSAGE(ret != 0, "MQTT Connect failed");

    for (uint8_t i=0; i<2; i++) {

        uint8_t mqtt_raw_buffer[64];
        asleep((1+(i*2))*1000);
        // MQTT PING REQ
        TEST_ASSERT_TRUE(mqtt_ping_req(&data_stream_out_fptr_) == Successfull);

        // Wait PINGRESP from borker
        int rcv = recv(g_socket_desc, mqtt_raw_buffer , sizeof(mqtt_raw_buffer), 0);
        TEST_ASSERT_FALSE_MESSAGE(rcv < 0,  "Receive failed with error");

        if (i == 0) {
            TEST_ASSERT_FALSE_MESSAGE(rcv == 0, "No data received");
            TEST_ASSERT_EQUAL(Successfull, mqtt_parse_ping_ack(mqtt_raw_buffer));
        } /* else {
            TEST_ASSERT_TRUE_MESSAGE(rcv == 0, "Data received??");
        } */
    }

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
    RUN_TEST(test_mqtt_connect_simple_keepalive,                tCntr++);
    RUN_TEST(test_mqtt_connect_simple_keepalive_timeout,        tCntr++);

    return (UnityEnd());
}
