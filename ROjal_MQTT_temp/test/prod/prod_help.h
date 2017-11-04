#ifndef PROD_HELP_H
#define PROD_HELP_H

#include "mqtt.h"
#include "unity.h"

bool enable_(uint16_t a_keepalive_timeout, char * clientName);
bool disable_();
void data_from_socket(uint8_t * a_data, size_t a_amount);

#endif //PROD_HELP_C
