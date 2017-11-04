
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>   //inet_addr
#include <netinet/tcp.h> // TCP_NODELAY
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "mqtt.h"
#include "unity.h"
#include "../help/help.h"

extern MQTTErrorCodes_t mqtt_connect_parse_ack(uint8_t * a_message_in_ptr);
extern bool g_auto_state_connection_completed_;
extern bool socket_OK_;

void test_sm_connect_auto_ack_keepalive()
{
    uint8_t buffer[1024];
    MQTT_shared_data_t shared;

    shared.buffer = buffer;
    shared.buffer_size = sizeof(buffer);
    shared.out_fptr = &data_stream_out_fptr_;
    shared.connected_cb_fptr = &connected_cb_;
    g_auto_state_connection_completed_ = false;

    MQTT_action_data_t action;
    action.action_argument.shared_ptr = &shared;
    MQTTErrorCodes_t state = mqtt(ACTION_INIT,
                                  &action);

    MQTT_connect_t connect_params;
    uint8_t clientid[] = "JAMKtest test_sm_connect_auto_ack_keepalive";
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

    action.action_argument.connect_ptr = &connect_params;

    state = mqtt(ACTION_CONNECT,
                 &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    asleep(10);
    // Wait response and request parse for it
    // Parse will call given callback which will set global flag to true
    int rcv = data_stream_in_fptr_(buffer, sizeof(MQTT_fixed_header_t));
    if (0 < rcv) {
        MQTT_input_stream_t input;
        input.data = buffer;
        input.size_of_data = (uint32_t)rcv;
        action.action_argument.input_stream_ptr = &input;

        state = mqtt(ACTION_PARSE_INPUT_STREAM,
                     &action);
    } else {
        TEST_ASSERT(0);
    }

    do {
        /* Wait callback */
        asleep(1);
    } while( false == g_auto_state_connection_completed_);

    MQTT_action_data_t ap;
    ap.action_argument.epalsed_time_in_ms = 500;
    state = mqtt(ACTION_KEEPALIVE, &ap);

    printf("Keepalive cmd status %i\n", state);
    ap.action_argument.epalsed_time_in_ms = 500;

    for (uint8_t i = 0; i < 10; i++)
    {
        printf("g_shared_data->state %u\n", shared.state);

        if (Successfull == state) {
            int rcv = data_stream_in_fptr_(buffer, sizeof(MQTT_fixed_header_t));
            if (0 < rcv) {
                MQTT_input_stream_t input;
                input.data = buffer;
                input.size_of_data = (uint32_t)rcv;
                action.action_argument.input_stream_ptr = &input;

                state = mqtt(ACTION_PARSE_INPUT_STREAM,
                            &action);
                TEST_ASSERT_EQUAL_INT(Successfull, state);
            } else {
                printf("no ping resp\n");
                state = Successfull;
                //TEST_ASSERT(0);
            }
        }

        printf("sleeping..\n");
        asleep(300); // 100ms safe guard because given time is at least time...
        printf("slept\n");
        state = mqtt(ACTION_KEEPALIVE, &ap);
        //TEST_ASSERT_EQUAL_INT(Successfull, state);

    }
    state = mqtt(ACTION_DISCONNECT,
                 NULL);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    close_mqtt_socket_();
}
/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("State Maschine keepalive");
    unsigned int tCntr = 1;
    RUN_TEST(test_sm_connect_auto_ack_keepalive,            tCntr++);
    return (UnityEnd());
}
