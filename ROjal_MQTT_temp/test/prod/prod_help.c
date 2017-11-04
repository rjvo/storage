
#include "prod_help.h"
#include "help.h"

static uint8_t mqtt_send_buffer[1024];
static MQTT_shared_data_t mqtt_shared_data;

#include "socket_read_write.h"

bool enable_(uint16_t a_keepalive_timeout, char * clientName)
{
    /* Open socket */
    bool ret = socket_initialize(MQTT_SERVER, MQTT_PORT, data_from_socket);

    if (ret) {
        /* Initialize MQTT */
        return mqtt_connect(clientName,
                            a_keepalive_timeout,
                            (uint8_t*)"\0",
                            (uint8_t*)"\0",
                            (uint8_t*)"\0",
                            (uint8_t*)"\0",
                            &mqtt_shared_data,
                            mqtt_send_buffer,
                            sizeof(mqtt_send_buffer),
                            false,
                            &socket_write,
                            &connected_cb_,
                            &subscrbe_cb_,
                            10);
    }
    return false;
}

bool disable_()
{
    TEST_ASSERT_TRUE_MESSAGE(mqtt_disconnect(), "MQTT Disconnect failed");
    printf("MQTT Disconnected\n");

    printf("Close socket\n");
    TEST_ASSERT_TRUE_MESSAGE(stop_reading_thread(), "SOCKET exit failed");
    //sleep_ms(2000);
    return true;
}

void data_from_socket(uint8_t * a_data, size_t a_amount)
{
    TEST_ASSERT_TRUE_MESSAGE(mqtt_receive(a_data, a_amount), "Receive failed (socket -> mqtt)");
}