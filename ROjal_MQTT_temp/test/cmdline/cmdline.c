#include <argp.h>    // http://www.gnu.org/software/libc/manual/html_node/Argp.html#Argp
#include <stdbool.h>
#include <stdint.h>  // uint
#include <stdlib.h>  // atoi
#include <string.h>  // strlen
#include <stdarg.h>  // tracing

#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>
#include<time.h>      //nanosleep

#include <signal.h>   // catch Ctrl + C signal

#include "mqtt.h"
#include "socket_read_write.h"

const char *argp_program_version = "ROjal_MQTT_Client v0.1";
static char doc[]                = "MQTT 3.1.1 Client supporting QoS0 level communication";
static char args_doc[]           = "rmc [FLAGS]";

static struct argp_option options[] = {
    { "topic",     't', "Topic",      0, "Topic to be subscribed or where to publish:", 0},
    { "message",   'm', "Message",    0, "Message in case of publish. If not defined = publish:", 0},
    { "file",      'f', "File",       0, "Send file", 0},
    { "receive",   'r', 0,            0, "Receive file", 0},
    { "broker",    'b', "IP",         0, "Broker IP address e.g. 192.168.0.1:", 0},
    { "clean",     'c', 0,            0, "Disable clean session(default is clean):", 0},
    { "keepalive", 'k', "sec",        0, "Keepalive in seconds (default = 0 = no keepalive):", 0},
    { "lastwill",  'w', "Will",       0, "Last will message:", 0},
    { "lasttopic", 'l', "WillTopic",  0, "Last will topic:", 0},
    { "client",    'n', "ClientID",   0, "Client ID, if not defined ROjal_MQTT_Client<random> is used:", 0},
    { "sport",     's', "SocketPort", 0, "MQTT's Socket port (if not defined 1883 will be used):", 0},
    { "user",      'u', "Username",   0, "Username (if required by broker):", 0},
    { "password",  'p', "Password",   0, "Password (if required by broker):", 0},
    { "verbose",   'v', 0,            0, "Verbose:", 0},
    { 0 }
};

struct arguments {
    uint8_t * message;
    uint8_t * topic;
    uint8_t * clientID;
    bool      clientid_set;
    uint8_t * hostip;
    uint8_t * last_will_message;
    uint8_t * last_will_topic;
    uint8_t * username;
    uint8_t * password;
    uint32_t  keepalive;
    bool      clean;
    uint32_t  hostport;
    uint8_t * filename;
    bool      receive_file;
    bool      verbose;
};

static MQTT_shared_data_t mqtt_shared_data;
static uint8_t            a_output_buffer[1024]; /* Shared buffer */

static struct arguments arguments; /* Argument prarsing script    */

static volatile int subscribe_continue = 1;

void ctrl_c_exit(int a_ignore) {
    a_ignore = a_ignore;
    printf("Cntr+C received - exit\n");
    subscribe_continue = 0;
}

void sleep_in_sec(int seconds)
{
    struct timespec ts;
    ts.tv_sec = seconds;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
    //nanosleep(&ts, NULL);
    //pselect(0, NULL, NULL, NULL, &ts, NULL);
    //thrd_sleep(&ts, NULL); // sleep 1 sec
}

int trace(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if(arguments.verbose)
        vprintf(format, args);

    va_end(args);

    return 0;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;

    if (arg) {
        switch (key) {
            case 't':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->topic   = p;
                break;
            }
            case 'm':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->message = p;
                break;
            }
            case 'b':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->hostip = p;
                break;
            }
            case 'n':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->clientID = p;
                arguments->clientid_set = true;
                break;
            }
            case 'u':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->username = p;
                break;
            }
            case 'p':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->password = p;
                break;
            }
            case 'f':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->filename = p;
                break;
            }
            case 'k':
            {
                int value = atoi(arg);
                if (0 < value)
                    arguments->keepalive = value;
                break;
            }
            case 's':
            {
                int value = atoi(arg);
                if (0 < value)
                    arguments->hostport = value;
                break;
            }
            case 'w':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->last_will_message = p;
                break;
            }
            case 'l':
            {
                size_t len = strlen(arg);
                uint8_t * p = (uint8_t*)malloc(len);
                memcpy(p, arg, len);
                arguments->last_will_topic = p;
                break;
            }
            case ARGP_KEY_ARG:
                return 0;
            default:
                return ARGP_ERR_UNKNOWN;
        }
    } else {
        switch (key) {
            case 'c':
            {
                arguments->clean = false;
                break;
            }
            case 'r':
            {
                printf(".....rec\n");
                arguments->receive_file = true;
                break;
            }
            case 'v':
            {
                arguments->verbose = true;
                break;
            }
            default:
                return ARGP_ERR_UNKNOWN;
        }
    }
    return 0;
}

void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec  = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void connected_cb(MQTTErrorCodes_t a_status)
{
    if (Successfull == a_status) {
        printf("Connected\n");
    } else {
        printf("Connection FAIL %i\n", a_status);
    }
}

void subscrbe_cb(MQTTErrorCodes_t   a_status,
                 uint8_t          * a_data_ptr,
                 uint32_t           a_data_len,
                 uint8_t          * a_topic_ptr,
                 uint16_t           a_topic_len)
{
    if (Successfull == a_status) {
        if (0 < a_data_len) {
            /* Save to file */
            if ('\0' != (char)(arguments.filename[0])) {
                FILE * fp = fopen((const char *)arguments.filename, "ab+");
                if (fp) {
                    fwrite(a_data_ptr, sizeof(uint8_t), a_data_len, fp);
                    fclose(fp);
                }
            }
            /* Print to screen */
            if (128 > a_data_len) {
                for (uint16_t i = 0; i < a_topic_len; i++)
                    printf("%c", a_topic_ptr[i]);
                printf(" [%i] ", a_data_len);
                for (uint32_t i = 0; i < a_data_len; i++)
                    printf("%c", a_data_ptr[i]);
                printf("\n");
            } else {
                for (uint16_t i = 0; i < a_topic_len; i++)
                    printf("%c", a_topic_ptr[i]);
                printf(" [%i]\n", a_data_len);
            }
        }
    } else {
        printf("Subscribed CB FAIL %i\n", a_status);
    }
}

void data_from_socket(uint8_t * a_data, size_t a_amount)
{
    mqtt_receive(a_data, a_amount);
}


void clean(uint8_t * a_data, char * a_param)
{
    if (1 < strlen((char*)a_data))
    {
        trace("\tClean %s\n", (char*)a_param);
        free((char*)a_data);
    }
}

bool rmc_connect(struct arguments * arguments)
{
    if (false == socket_initialize((char*)(arguments->hostip), arguments->hostport, &data_from_socket))
        return false;
    return mqtt_connect((char *)arguments->clientID,
                                arguments->keepalive,
                                arguments->username,
                                arguments->password,
                                arguments->last_will_topic,
                                arguments->last_will_message,
                                &mqtt_shared_data,
                                a_output_buffer,
                                sizeof(a_output_buffer),
                                arguments->clean,
                                &socket_write,
                                &connected_cb,
                                &subscrbe_cb,
                                10);
}

void rmc_disconnect()
{
    mqtt_disconnect();
}


int main(int argc, char *argv[])
{
    struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

    /* Generate random Client ID */
    srand(time(NULL));               // Seed to random
    int random = rand() % 1000 + 1;  // Random in range 1-1000
    uint8_t tempClientID[128] = "";
    sprintf((char*)tempClientID, "ROjal_MQTT_Client%i", random);

    uint8_t empty[]             = "\0";

    arguments.keepalive         = 0;
    arguments.clean             = true;
    arguments.clientid_set      = false;
    arguments.clientID          = tempClientID;
    arguments.message           = empty;
    arguments.topic             = empty;
    arguments.hostip            = empty;
    arguments.last_will_message = empty;
    arguments.last_will_topic   = empty;
    arguments.username          = empty;
    arguments.password          = empty;
    arguments.hostport          = 1883;
    arguments.filename          = empty;
    arguments.receive_file      = false;
    arguments.verbose           = false;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    trace("\tBroker    %s:%i\n", arguments.hostip, arguments.hostport);
    trace("\tUsername  %s\n",    arguments.username);
    trace("\tPassword  %s\n",    arguments.password);
    trace("\tKeepalive %i\n",    arguments.keepalive);
    trace("\tClean     %i\n",    arguments.clean);
    trace("\tReveive   %i\n",    arguments.receive_file);
    trace("\tClientID  %s\n",    arguments.clientID);
    trace("\tMessage   %s\n",    arguments.message);
    trace("\tFilename  %s\n",    arguments.filename);
    trace("\tTopic     %s\n",    arguments.topic);
    trace("\tLWT       %s\n",    arguments.last_will_message);
    trace("\tLWT topic %s\n",    arguments.last_will_topic);

    bool valid_parameters = true;

    if (0 == strlen((char*)(arguments.hostip))) {
        printf("Broker IP must be defined\n");
        valid_parameters = false;
    }

    if (0 == strlen((char*)(arguments.topic))) {
        printf("Topic must be defined\n");
        valid_parameters = false;
    }

    if (valid_parameters) {
        if (rmc_connect(&arguments)) {
            if (((0 == strlen((char*)(arguments.message)))  && // No message & No filename & no receive
                 (0 == strlen((char*)(arguments.filename))) &&
                 (false == arguments.receive_file))         || // or filename and receive
                ((0 < strlen((char*)(arguments.filename)))  &&
                 (true == arguments.receive_file))) {

                printf("Subscribe\n");
                if (true == mqtt_subscribe((char *)arguments.topic, strlen((char*)(arguments.topic)), 10)) {

                    signal(SIGINT, ctrl_c_exit);

                    while (subscribe_continue) {
                        if ( 0 < arguments.keepalive ) {
                            sleep_in_sec(arguments.keepalive); // Sleep seconds
                            printf("keepalive...\n");
                            fflush(stdout);
                            if (subscribe_continue)
                                subscribe_continue = mqtt_keepalive(arguments.keepalive * 1000); // Update in ms
                        } else {
                            sleep_in_sec(1);
                        }
                    }
                } else {
                    printf("Subscribe failed\nExit...\n");
                }

            } else {
                if (0 < strlen((char*)(arguments.message))) {

                    printf("Publish MSG\n");
                    mqtt_publish((char *)arguments.topic,
                                  strlen((char*)(arguments.topic)),
                                 (char *)arguments.message,
                                  strlen((char*)(arguments.message)));
                } else {

                    if (0 < strlen((char*)(arguments.filename))) {

                        FILE * f = fopen((char*)(arguments.filename), "r");
                        if (NULL != f) {
                            fseek(f, 0, SEEK_END); // End of the file
                            unsigned long len = (unsigned long)ftell(f);
                            fseek(f, 0, SEEK_SET); // Beginning of the file
                            char * buf = malloc(len + 1);
                            if(len != fread(buf, len, 1, f)) {
                                fclose(f);
                                uint32_t mqttbuf_size = len +
                                                        strlen((char*)(arguments.topic)) +
                                                        sizeof(MQTT_fixed_header_t) +
                                                        64 /* Spare */;

                                uint8_t * mqttbuf = (uint8_t *)malloc(mqttbuf_size);

                                printf("Sending file %s [%lu Bytes] %p\n", (char*)(arguments.filename), len, a_output_buffer);
                                printf("Status: %i\n", mqtt_publish_buf((char *)arguments.topic,
                                                                        strlen((char*)(arguments.topic)),
                                                                        buf,
                                                                        len,
                                                                        mqttbuf,
                                                                        mqttbuf_size));

                                free(mqttbuf);
                            } else {
                                printf("Failed to read file %s\n", (char*)(arguments.filename));
                            }
                            free(buf);
                        } else {
                            printf("Failed to open %s\n", arguments.filename);
                        }
                    } else {
                        printf("No message or filename given - nothing to send\n");
                    }
                }
            }
            rmc_disconnect();
        }
    }

    trace("\n\n--------------------\nCleanUp\n");
    if (arguments.clientid_set)
        clean(arguments.clientID,      "ClientID");

    clean(arguments.hostip,            "Broker");
    clean(arguments.message,           "Message");
    clean(arguments.filename,          "Filename");
    clean(arguments.topic,             "Topic");
    clean(arguments.last_will_message, "LWT");
    clean(arguments.last_will_topic,   "LWT Topic");
    clean(arguments.username,          "Username");
    clean(arguments.password,          "Password");
    trace("--------------------\n");
}
