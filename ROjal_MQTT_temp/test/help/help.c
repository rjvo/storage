#include "help.h"
#include "unity.h"

//#define _POSIX_C_SOURCE       199309L
#include <time.h>      //nanosleep
//#include <threads.h>
#include <stdio.h>
#include <sys/select.h>
#include <unistd.h>


#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <signal.h>


volatile bool g_auto_state_connection_completed_ = false;
volatile bool g_auto_state_subscribe_completed_  = false;

static int g_socket_desc = -1;
bool socket_OK_          = false;

void socket_sig_handler()
{
    printf("Socket signal\n");
    socket_OK_ = false;
}

void asleep(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec  = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
    //nanosleep(&ts, NULL);
    //pselect(0, NULL, NULL, NULL, &ts, NULL);
    //thrd_sleep(&ts, NULL); // sleep 1 sec
}

int open_mqtt_socket_()
{
    struct sockaddr_in server;
    //Create socket

    // catch ctrl c
    signal(SIGPIPE, socket_sig_handler);

    g_socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    TEST_ASSERT_NOT_EQUAL( -1, g_socket_desc);

    server.sin_addr.s_addr = inet_addr(MQTT_SERVER);
    server.sin_family      = AF_INET;
    server.sin_port        = htons(MQTT_PORT);

    printf("MQTT server %s:%i\n", MQTT_SERVER, MQTT_PORT);

    //Connect to remote server
    TEST_ASSERT_TRUE_MESSAGE(connect(g_socket_desc, (struct sockaddr *)&server, sizeof(server)) >= 0,
                             "MQTT Broker not running?");

    socket_OK_ = true;
    return g_socket_desc;
}

void close_mqtt_socket_()
{
    bool closing = true;
    printf("Socket closing\n");
    int din = 1;
    while (din < 0){
        int a_data_ptr =0 ;
        din = recv(g_socket_desc, &a_data_ptr, 1, 0);
        }

    //http://www.gnu.org/software/libc/manual/html_node/Closing-a-Socket.html
    shutdown(g_socket_desc, 2 /* Ignore and stop both RCV and SEND */);

    while (closing) {
        asleep(10);
        char data = 0;
        if (send(g_socket_desc, &data, 1 , 0) < 0)
            closing = false;
    }
    printf("Socket closed\n");
    g_socket_desc = -1;
    socket_OK_ = false;
}

int data_stream_in_fptr_(uint8_t * a_data_ptr, size_t a_amount)
{
    if (g_socket_desc <= 0)
        g_socket_desc = open_mqtt_socket_(&g_socket_desc);

    if (g_socket_desc > 0) {
        int ret = recv(g_socket_desc, a_data_ptr, a_amount, 0);
        printf("Socket in  -> %iB\n", ret);
        return ret;
    } else {
        return -1;
    }
}

int data_stream_out_fptr_(uint8_t * a_data_ptr, size_t a_amount)
{
    if (g_socket_desc <= 0)
        g_socket_desc = open_mqtt_socket_(&g_socket_desc);

    if (g_socket_desc > 0) {
        int ret = send(g_socket_desc, a_data_ptr, a_amount, 0);
        printf("Socket out -> %iB\n", ret);
        return ret;
    } else {
        return -1;
    }
}

void connected_cb_(MQTTErrorCodes_t a_status)
{
    if (Successfull == a_status)
        printf("Connected CB SUCCESSFULL\n");
    else
        printf("Connected CB FAIL %i\n", a_status);
    g_auto_state_connection_completed_ = true;
}

void subscrbe_cb_(MQTTErrorCodes_t a_status,
                 uint8_t * a_data_ptr,
                 uint32_t a_data_len,
                 uint8_t * a_topic_ptr,
                 uint16_t a_topic_len)
{
    if (Successfull == a_status) {
        printf("Subscribed CB SUCCESSFULL\n");

        for (uint16_t i = 0; i < a_topic_len; i++)
            printf("%c", a_topic_ptr[i]);
        printf(": ");

        for (uint32_t i = 0; i < a_data_len; i++)
            printf("%c", a_data_ptr[i]);
        printf("\n");

    } else {
        printf("Subscribed CB FAIL %i\n", a_status);
    }
    g_auto_state_subscribe_completed_ = true;
}
