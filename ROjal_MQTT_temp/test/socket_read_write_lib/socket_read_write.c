#include <stdio.h>      // printf
#include <sys/socket.h> // socket
#include <unistd.h>     // socket / file close
#include <arpa/inet.h>  // inet_addr
#include <pthread.h>    // pthread_create
#include <signal.h>     // pthread_kill
#include <stdlib.h>     // malloc/free
#include <string.h>     // memcpy
#include "socket_read_write.h"

static int test_socket = -1;
static socket_data_received_fptr_t socket_data_received_callback;
static pthread_t socket_reading_thread_id = -1;

static bool socket_OK = false;
static volatile bool read_thread_running = true;

void ctrlc_handler()
{
    socket_OK = false;
}

#define BUFFER_SIZE (1024*1024)

int socket_write(uint8_t * a_data, size_t a_amount)
{
    return send(test_socket, a_data, a_amount , 0);
}

void sleep_ms_(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec  = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int32_t get_remainingsize(uint8_t * a_input_ptr)
{
    uint32_t multiplier = 1;
    uint32_t value      = 0;
    uint8_t  cnt        = 1;
    uint8_t  aByte      = 0;

    /* Verify input parameters */
    if (NULL == a_input_ptr)
        return -1;

    do {
        aByte = a_input_ptr[cnt++];
        value += (aByte & 127) * multiplier;
        if (multiplier > (128*128*128))
            return -2;

        multiplier *= 128;
    } while ((aByte & 128) != 0);
    value += (cnt); //Add count header to the overal size of MQTT packet to be received
    return value;
}

void read_signal_handler()
{
    printf("Killing reading thread\n");
    read_thread_running = false;
}

void *socket_receive_thread(void * a_ptr)
{
    a_ptr = a_ptr;
    read_thread_running = true;
    signal(SIGUSR1, read_signal_handler);

    while (((test_socket) > 0)                     &&
           (NULL != socket_data_received_callback) &&
           (read_thread_running)) {

        char header[32] = {0};
        int bytes_read = recv((test_socket), header, sizeof(header) - 1, 0);

        if (2 <= bytes_read) {
            uint32_t remaining_bytes = get_remainingsize((uint8_t*)header) - bytes_read;

        if (0 < remaining_bytes) {

            /* Need to read more data */
            char * buff = (char*)malloc(remaining_bytes + bytes_read + 1024 /* safety buffer*/);
            memcpy(buff, header, bytes_read);
            while (remaining_bytes) {
                int nxt_bytes_read = recv(test_socket, &buff[bytes_read], remaining_bytes, 0);
                remaining_bytes -= nxt_bytes_read;
                bytes_read += nxt_bytes_read;
            }

            socket_data_received_callback((uint8_t*)buff, (uint32_t)bytes_read);
            free(buff);
            } else {
                /* Send packets dirctly to the MQTT client, which fit into 32B buffer */
                socket_data_received_callback((uint8_t*)header, (uint32_t)bytes_read);
            }
        } else {
            char data = 0;
            if( send(test_socket, &data, 0 , 0) < 0)
                return 0;
        }
    }
    return 0;
}

bool socket_initialize(char * a_inet_addr, uint32_t a_port, socket_data_received_fptr_t a_receive_callback)
{
    struct sockaddr_in server;
    // catch ctrl c
    signal(SIGPIPE, ctrlc_handler);

    //Create socket
    test_socket = socket(AF_INET , SOCK_STREAM, 0);
    if (-1 == test_socket) {
        return false;
    }

    int value = 1;
    setsockopt(test_socket, SOL_SOCKET, SO_REUSEADDR,&value, sizeof(int)); // https://stackoverflow.com/questions/10619952/how-to-completely-destroy-a-socket-connection-in-c

    socket_OK = true;

    struct timeval timeout;
        timeout.tv_sec  = 10;
        timeout.tv_usec = 0;

    if (setsockopt (test_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                    sizeof(timeout)) < 0)
        printf("SO_RCVTIMEO failed\n");

    if (setsockopt (test_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                    sizeof(timeout)) < 0)
        printf("SO_SNDTIMEO failed\n");

    server.sin_addr.s_addr = inet_addr(a_inet_addr);
    server.sin_family      = AF_INET;
    server.sin_port        = htons(a_port);

    //Connect to remote server
    if (connect(test_socket , (struct sockaddr *)&server , sizeof(server)) < 0)
        return false;

    socket_data_received_callback = a_receive_callback;

    if (pthread_create( &socket_reading_thread_id, NULL, socket_receive_thread, (void*) &test_socket) < 0)
        return false;

    return true;
}

bool stop_reading_thread()
{
    printf("Stoping socket...\n");
    if (0 < test_socket) {
            //close(test_socket);
            shutdown(test_socket, 2 /* Ignore and stop both RCV and SEND */); //http://www.gnu.org/software/libc/manual/html_node/Closing-a-Socket.html
            while(socket_OK) {
                sleep_ms_(100);
                char data = 0;
                if(send(test_socket, &data, 1 , 0) < 0)
                   socket_OK = false;
            }
            test_socket = -1;
            printf("Socket closed...\n");
            // https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.2.0/com.ibm.zos.v2r2.bpxbd00/ptkill.htm
            pthread_kill(socket_reading_thread_id, SIGUSR1);
            // pthread_join(socket_reading_thread_id, NULL);
            printf("Stoping closing completed\n");
            socket_reading_thread_id = -1;
            return true;
    }
    return true;
}

