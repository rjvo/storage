#include "mqtt.h"

int main()
{
    uint8_t a_buffer[1];
    static MQTT_shared_data_t mqtt_shared_data;
    mqtt_connect("Empty", 0, '\0','\0','\0','\0',
                 &mqtt_shared_data,
                 a_buffer,
                 sizeof(a_buffer),
                 0,
                 NULL,
                 NULL,
                 NULL,
                 0);
}
