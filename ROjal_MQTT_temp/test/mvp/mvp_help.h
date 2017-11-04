#ifndef MVP_HELP_H
#define MVP_HELP_H

#include "mqtt.h"

bool mvp_keepalive_(uint32_t a_duration_in_ms);
bool mvp_enable_(uint16_t a_keepalive_timeout, char * clientName);
bool mvp_disable_();
void mvp_connected_cb_(MQTTErrorCodes_t a_status);
void mvp_data_from_socket_(uint8_t * a_data, size_t a_amount);
bool mvp_publish_(char * a_topic, char * a_msg);
bool mvp_subscribe_(char * a_topic);

#endif //MVP_HELP_H
