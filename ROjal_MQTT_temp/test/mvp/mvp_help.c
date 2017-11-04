#include "mvp_help.h"
#include "mqtt.h"
#include "help.h"
#include "socket_read_write.h"
#include "unity.h"

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
#include <string.h>

static uint8_t            mqtt_send_buffer[1024*10];
static MQTT_shared_data_t mqtt_shared_data;

static bool g_mqtt_connected  = false;
static bool g_mqtt_subscribed = false;
bool mvp_subscribe_test2      = false;

bool mvp_keepalive_(uint32_t a_duration_in_ms)
{
    MQTT_action_data_t ap;
    ap.action_argument.epalsed_time_in_ms = a_duration_in_ms;
    mqtt(ACTION_KEEPALIVE, &ap);
    return true;
}

void mvp_connected_cb_(MQTTErrorCodes_t a_status)
{
    if (Successfull == a_status) {
        printf("Connected CB SUCCESSFULL\n");
    } else {
        printf("Connected CB FAIL %i\n", a_status);
    }
    g_mqtt_connected = true;
}

void mvp_subscribe_cb_(MQTTErrorCodes_t   a_status,
                       uint8_t          * a_data_ptr,
                       uint32_t           a_data_len,
                       uint8_t          * a_topic_ptr,
                       uint16_t           a_topic_len)
{
    if (Successfull == a_status) {
        printf("Subscribed CB SUCCESSFULL\n");
        for (uint16_t i = 0; i < a_topic_len; i++)
            printf("%c", a_topic_ptr[i]);

        printf(": ");
        for (uint32_t i = 0; i < a_data_len; i++)
            printf("%c", a_data_ptr[i]);
        printf("\n");

        if (memcmp("mvp/test2", a_topic_ptr, a_topic_len) == 0) {
            if (memcmp("MVP testing", a_data_ptr, a_data_len ) == 0)
                mvp_subscribe_test2 = true;
        }

    } else {
        printf("Subscribed CB FAIL %i\n", a_status);
    }
    g_mqtt_subscribed = true;
}

bool mvp_enable_(uint16_t a_keepalive_timeout, char * clientName)
{
    /* Open socket */
    bool ret = socket_initialize(MQTT_SERVER, MQTT_PORT, mvp_data_from_socket_);

    if (ret) {
        /* Initialize MQTT */
        mqtt_shared_data.buffer            = mqtt_send_buffer;
        mqtt_shared_data.buffer_size       = sizeof(mqtt_send_buffer);
        mqtt_shared_data.out_fptr          = &socket_write;
        mqtt_shared_data.connected_cb_fptr = &mvp_connected_cb_;
        mqtt_shared_data.subscribe_cb_fptr = &mvp_subscribe_cb_;

        MQTT_action_data_t action;
        action.action_argument.shared_ptr  = &mqtt_shared_data;

        MQTTErrorCodes_t state = mqtt(ACTION_INIT,
                                      &action);

        TEST_ASSERT_EQUAL_INT(Successfull, state);
        printf("MQTT Initialized\n");

        /* Connect to broker */
        MQTT_connect_t connect_params;
        connect_params.client_id = (uint8_t*)clientName;
        uint8_t aparam[]         = "\0";

        connect_params.last_will_topic              = aparam;
        connect_params.last_will_message            = aparam;
        connect_params.username                     = aparam;
        connect_params.password                     = aparam;
        connect_params.keepalive                    = a_keepalive_timeout;
        connect_params.connect_flags.clean_session  = true;
        connect_params.connect_flags.last_will_qos  = 0;
        connect_params.connect_flags.permanent_will = false;

        action.action_argument.connect_ptr = &connect_params;

        state = mqtt(ACTION_CONNECT,
                     &action);

        TEST_ASSERT_EQUAL_INT(Successfull, state);
        printf("MQTT Connecting\n");

        uint8_t timeout = 50;
        while (timeout != 0 && g_mqtt_connected == false) {
            timeout--;
            asleep(100);
        }

        printf("MQTT connack or timeout %i %i\n", g_mqtt_connected, timeout);
        if (g_mqtt_connected == false)
            ret = false;

    } else {
        printf("Socket failed\n");
    }
    g_mqtt_connected = false;
    return ret;
}

bool mvp_disable_()
{
    MQTTErrorCodes_t state = mqtt(ACTION_DISCONNECT,
                                  NULL);

    TEST_ASSERT_EQUAL_INT(Successfull, state);
    printf("MQTT Disconnected\n");

    printf("Close socket\n");
    TEST_ASSERT_TRUE_MESSAGE(stop_reading_thread(), "SOCKET exit failed");
    return true;
}

void mvp_data_from_socket_(uint8_t * a_data, size_t a_amount)
{
    /* Parse input messages */
    MQTT_input_stream_t input;
    input.data         = a_data;
    input.size_of_data = (uint32_t)a_amount;

    MQTT_action_data_t action;
    action.action_argument.input_stream_ptr = &input;

    MQTTErrorCodes_t state = mqtt(ACTION_PARSE_INPUT_STREAM,
                                  &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);
}


bool mvp_publish_(char * a_topic, char * a_msg)
{
    MQTT_publish_t publish;
    publish.flags.dup                  = false;
    publish.flags.retain               = false;
    publish.flags.qos                  = QoS0;
    publish.topic_ptr                  = (uint8_t*)a_topic;
    publish.topic_length               = strlen(a_topic);
    publish.message_buffer_ptr         = (uint8_t*)a_msg;
    publish.message_buffer_size        = strlen(a_msg);
    publish.output_buffer_ptr          = NULL;
    publish.output_buffer_size         = 0;

    MQTT_action_data_t action;
    action.action_argument.publish_ptr = &publish;

    MQTTErrorCodes_t state = mqtt(ACTION_PUBLISH,
                                  &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);
    return true;
}

bool mvp_subscribe_(char * a_topic)
{
    bool ret = false;

    MQTT_subscribe_t subscribe;
    subscribe.qos = QoS0;

    subscribe.topic_ptr    = (uint8_t*) a_topic;
    subscribe.topic_length = strlen(a_topic);

    MQTT_action_data_t action;
    action.action_argument.subscribe_ptr = &subscribe;

    MQTTErrorCodes_t state = mqtt(ACTION_SUBSCRIBE,
                                  &action);

    TEST_ASSERT_EQUAL_INT(Successfull, state);

    uint8_t timeout = 50;
    while (timeout != 0 && g_mqtt_subscribed == false) {
        timeout--;
        asleep(100);
    }
    if (true == g_mqtt_subscribed)
        ret = true;

    g_mqtt_subscribed = false;

    return ret;
}