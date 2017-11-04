#ifndef SOCKET_READ_WRITE_H
#define SOCKET_READ_WRITE_H

#include <stdint.h>  // uint
#include <stdbool.h> // bool

typedef void (*socket_data_received_fptr_t)(uint8_t * a_data, size_t amount);

bool socket_initialize(char * a_inet_addr, uint32_t a_port, socket_data_received_fptr_t);
int socket_write(uint8_t * a_data, size_t a_amount);
bool stop_reading_thread();

#endif