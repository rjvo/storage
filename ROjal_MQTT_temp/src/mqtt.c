/************************************************************************************************************
 * \subsection ROjal_MQTT_Client_Src MQTT Client source code                                                *
 *                                                                                                          *
 * Copyright 2017 Rami Ojala / JAMK (K5643)                                                                 *
 *                                                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of                          *
 * this software and associated documentation files (the "Software"), to deal in the                        *
 * Software without restriction, including without limitation the rights to use, copy,                      *
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,                      *
 * and to permit persons to whom the Software is furnished to do so, subject to the                         *
 * following conditions:                                                                                    *
 *                                                                                                          *
 *  The above copyright notice and this permission notice shall be included                                 *
 *  in all copies or substantial portions of the Software.                                                  *
 *                                                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,                      *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A                            *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT                       *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION                        *
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE                           *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                                   *
 *                                                                                                          *
 * https://opensource.org/licenses/MIT                                                                      *
 ************************************************************************************************************/

#include "mqtt.h"

static MQTT_shared_data_t * g_shared_data = NULL;

/************************************************************************************************************
 *                                                                                                          *
 * \subsection Internal Declaration of local functions                                                      *
 *                                                                                                          *
 ************************************************************************************************************/

 /**
 * Decode connack from variable header stream.
 *
 * Parse out connection status from connak message.
 *
 * @param a_message_buffer_ptr [out] allocated working space.
 * @param a_max_buffer_size [in] maximum size of the working space.
 * @param a_in_fptr [in] input stream callback function (receive).
 * @param a_out_fptr [in] output stream callback function (send).
 * @param a_connect_ptr [in] connection parameters @see MQTT_connect_t.
 * @param wait_and_parse_response [in] when true, function will wait connak response from the broker and parse it.
 * @return pointer to input buffer from where next header starts to. NULL in case of failure.
 */
MQTTErrorCodes_t mqtt_connect_(uint8_t                * a_message_buffer_ptr,
                               size_t                   a_max_buffer_size,
                               data_stream_in_fptr_t    a_in_fptr,
                               data_stream_out_fptr_t   a_out_fptr,
                               MQTT_connect_t         * a_connect_ptr,
                               bool                     wait_and_parse_response);

/**
 * Fill connect parameters
 *
 * Function is called by mqtt_connect to fill connect parameters.
 *
 * @param a_message_buffer_ptr [out] allocated working space.
 * @param a_max_buffer_size [in] maximum size of the working space.
 * @param a_connect_ptr [in] connection parameters @see MQTT_connect_t.
 * @param a_ouput_size_ptr [out] size of the message.
 * @return pointer next header or NULL in case of failure.
 */
uint8_t * mqtt_connect_fill(uint8_t        * a_message_buffer_ptr,
                            size_t           a_max_buffer_size,
                            MQTT_connect_t * a_connect_ptr,
                            uint16_t       * a_ouput_size_ptr);

/**
 * Send MQTT disconnect.
 *
 * Send out MQTT disconnect message using the given callback.
 *
 * @param a_out_fptr [in] output stream callback function.
 * @return error code @see MQTTErrorCodes_t.
 */
MQTTErrorCodes_t mqtt_disconnect_(data_stream_out_fptr_t a_out_fptr);

/**
 * Send PING request.
 *
 * Construct ping request by building fixed header and send data out by calling
 * given funciton pointer.
 *
 * @param a_out_fptr [in] output stream callback function.
 * @return error code @see MQTTErrorCodes_t.
 */
MQTTErrorCodes_t mqtt_ping_req(data_stream_out_fptr_t a_out_fptr);

/**
 * Validate connection response.
 *
 * Connect ack message is validated by this funciton.
 *
 * @param a_message_in_ptr [in] start of MQTT message.
 * @return error code @see MQTTErrorCodes_t.
 */
MQTTErrorCodes_t mqtt_connect_parse_ack(uint8_t * a_message_in_ptr);

/**
 * Set size into fixed header.
 *
 * Construct size based on formula presented in MQTT protocol specification:
 * @see http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf chapter 2.2.3.
 *
 * @param a_output_ptr [out] pre-allocated puffer where size data will be written.
 * @param a_message_size [in] remaining size of MQTT message.
 * @return size of remaining length field in fixed header. 0 in case of failure.
 */
uint32_t set_size(MQTT_fixed_header_t * a_output_ptr,
                  size_t                a_message_size);

/**
 * Get size from received MQTT message.
 *
 * Destruct size based on formula presented in MQTT protocol specification:
 * @see http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf chapter 2.2.3.
 *
 * @param a_input_ptr [in] pointer to start of received MQTT message.
 * @param a_message_size_ptr [out] remaining size of MQTT message will be written to here.
 * @return pointer to next header or NULL in case of failure.
 */
uint8_t * get_size(uint8_t  * a_input_ptr,
                   uint32_t * a_message_size_ptr);


/************************************************************************************************************
 *                                                                                                          *
 * \subsection DecideInt Declaration of local decode functions                                              *
 *                                                                                                          *
 ************************************************************************************************************/

/**
 * Decode fixed header from input stream.
 *
 * Fixed header flags, message type and size are set by
 * this function. Result is stored to pre-allocated
 * output buffer.
 *
 * @param a_input_ptr [in] point to first byte of received MQTT message.
 * @param a_dup_ptr [out] duplicate bit.
 * @param a_qos_ptr [out] quality of service value @see MQTTQoSLevel_t.
 * @param a_retain_ptr [out] retain bit.
 * @param a_message_type_ptr [out] message type @see MQTTMessageType_t.
 * @param a_message_size_ptr [out] message folowed by the fixed header in bytes.
 * @return pointer to input buffer from where next header starts to. NULL in case of failure.
 */
uint8_t * decode_fixed_header(uint8_t           * a_input_ptr,
                              bool              * a_dup_ptr,
                              MQTTQoSLevel_t    * a_qos_ptr,
                              bool              * a_retain_ptr,
                              MQTTMessageType_t * a_message_type_ptr,
                              uint32_t          * a_message_size_ptr);

/**
 * Decode connack from variable header stream.
 *
 * Parse out connection status from connak message.
 *
 * @param a_input_ptr [in] point to first byte of received MQTT message.
 * @param a_connection_state_ptr [out] connection state. 0 = successfully connected.
 * @return pointer to input buffer from where next header starts to. NULL in case of failure.
 */
uint8_t * decode_variable_header_conack(uint8_t * a_input_ptr,
                                        uint8_t * a_connection_state_ptr);

/**
 * Decode variable header suback frame.
 *
 * Decode variable header suback frame which is received after subscribe command has been sent to
 * broker. Variable header contains subscribe status information for the subscribe request.
 *
 * @param a_input_ptr [in] point to first byte of variable header.
 * @param a_subscribe_state_ptr [out] Result of subscribe will be stored into this variable location.
 */
void decode_variable_header_suback(uint8_t          * a_input_ptr,
                                   MQTTErrorCodes_t * a_subscribe_state_ptr);

/**
 * Decode variable header publish frame.
 *
 * Decode variable header suback frame from given input stream. Varialbe header contains
 * topic name, length and topic quality level, which are parsed out from the given byte stream.
 *
 * @param a_input_ptr [in] point to first byte of variable header.
 * @param a_topic_out_ptr [out] this will point to location from where topic start on input stream.
 * @param a_qos [out] not used at this point, would contain QoS information of published message.
 * @param a_topic_length [out] topic length is written to to this parameter.
 * @return pointer to input buffer from where payload starts. NULL in case of failure.
 */
uint8_t * decode_variable_header_publish(uint8_t        *  a_input_ptr,
                                         uint8_t        ** a_topic_out_ptr,
                                         MQTTQoSLevel_t    a_qos,
                                         uint16_t       *  a_topic_length);

/**
 * Decode complete publish message.
 *
 * Decode publish message using fixed header and variable header functions.
 *
 * @param a_message_in_ptr [in] pointer to variable header part of a MQTT publish message.
 * @param a_size_of_msg [in] size of message.
 * @param a_qos [out] not used at this point, would contain QoS information of published message.
 * @param a_topic_out_ptr [out] will point to beginning of topic in given input stream.
 * @param a_topic_length_out_ptr [out] topic length is written to to this parameter.
 * @param a_out_message_ptr [out] will point to beginning of payload in given input stream.
 * @param a_out_message_size_ptr [out] payload length is written to to this parameter.
 * @return pointer to input buffer from where payload starts. NULL in case of failure.
 */
bool decode_publish(uint8_t        *  a_message_in_ptr,
                    uint32_t          a_size_of_msg,
                    MQTTQoSLevel_t    a_qos,
                    uint8_t        ** a_topic_out_ptr,
                    uint16_t       *  a_topic_length_out_ptr,
                    uint8_t        ** a_out_message_ptr,
                    uint32_t       *  a_out_message_size_ptr);


/************************************************************************************************************
 *                                                                                                          *
 * \subsection EncodeInt Declaration for internal encode functions                                          *
 *                                                                                                          *
 ************************************************************************************************************/

 /**
 * Construct variable header for connect message
 *
 * Fill in all fileds needed to build MQTT connect message.
 *
 * @param a_output_ptr [out] preallocated buffer where data is filled
 * @param a_clean_session [in] clean session bit
 * @param a_last_will_qos [in] QoS for last will @see MQTTQoSLevel_t
 * @param a_permanent_last_will [in] is last will permanent
 * @param a_password [in] payload contains password
 * @param a_username [in] payoad contains username
 * @param a_keepalive [in] 16bit keep alive counter
 * @return length of header = 10 bytes or 0 in case of failure
 */
uint8_t encode_variable_header_connect(uint8_t        * a_output_ptr,
                                       bool             a_clean_session,
                                       bool             a_last_will,
                                       MQTTQoSLevel_t   a_last_will_qos,
                                       bool             a_permanent_last_will,
                                       bool             a_password,
                                       bool             a_username,
                                       uint16_t         a_keepalive);

/**
 * Decode variable header publish frame.
 *
 * Decode variable header suback frame from given input stream. Varialbe header contains
 * topic name, length and topic quality level, which are parsed out from the given byte stream.
 *
 * @param a_input_ptr [in] point to first byte of variable header
 * @param a_topic_out_ptr [out] this will point to location from where topic start on input stream.
 * @param a_qos [out] not used at this point, would contain QoS information of published message.
 * @param a_topic_length [out] topic length is written to to this parameter.
 * @return pointer to input buffer from where payload starts. NULL in case of failure.
 */
bool encode_publish(data_stream_out_fptr_t   a_out_fptr,
                    uint8_t                * a_output_ptr,
                    uint32_t                 a_output_size,
                    bool                     a_retain,
                    MQTTQoSLevel_t           a_qos,
                    bool                     a_dup,
                    uint8_t                * topic_ptr,
                    uint16_t                 topic_size,
                    uint16_t                 packet_identifier,
                    uint8_t                * message_ptr,
                    uint32_t                 message_size);

 /**
 * Construct fixed header from given parameters.
 *
 * Fixed header flags, message type and size are set by
 * this function. Result is stored to pre-allocated
 * output buffer.
 *
 * @param a_output_ptr [out] is filled by the function (caller shall allocate and release)
 * @param a_dup [in] duplicate bit
 * @param a_qos [in] quality of service value @see MQTTQoSLevel_t
 * @param a_retain [in] retain bit
 * @param a_messageType [in] message type @see MQTTMessageType_t
 * @param a_msgSize [in] message folowed by the fixed header in bytes
 * @return size of header and 0 in case of failure
 */
uint8_t encode_fixed_header(MQTT_fixed_header_t * a_output_ptr,
                            bool                  a_dup,
                            MQTTQoSLevel_t        a_qos,
                            bool                  a_retain,
                            MQTTMessageType_t     a_messageType,
                            uint32_t              a_msgSize);

 /**
 * Construct fixed header from given parameters.
 *
 * Fixed header flags, message type and size are set by
 * this function. Result is stored to pre-allocated
 * output buffer.
 *
 * @param a_out_fptr [in] function pointer, which is called to send message out.
 * @param a_output_ptr [out] ouptut buffer, where date is stored before sending (caller ensure validity).
 * @param a_output_size [in] maximum size of given output buffer.
 * @param a_topic_qos [in] QoS for the topic.
 * @param a_topic_ptr [in] poitner to topic.
 * @param a_topic_size [in] size of the topic.
 * @param a_packet_identifier [in] packet sequence number.
 * @return true or false
 */
bool encode_subscribe(data_stream_out_fptr_t   a_out_fptr,
                      uint8_t                * a_output_ptr,
                      uint32_t                 a_output_size,
                      MQTTQoSLevel_t           a_topic_qos,
                      uint8_t                * a_topic_ptr,
                      uint16_t                 a_topic_size,
                      uint16_t                 a_packet_identifier);


/************************************************************************************************************
 *                                                                                                          *
 * \subsection TestAndDebug Test and debug functions                                                        *
 *                                                                                                          *
 ************************************************************************************************************/
/* Debug hex print function */
void hex_print(uint8_t * a_data_ptr, size_t a_size)
{
    #ifdef DEBUG
        if (a_size > 1024) /* Limit hex print to 1kB */
            a_size = 1024;
        for (size_t i = 0; i < a_size; i++)
            mqtt_printf("0x%02x ", a_data_ptr[i] & 0xff);

        mqtt_printf("\n");
    #else
        a_data_ptr = a_data_ptr;
        a_size = a_size;
    #endif
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection Size MQTT size get and set                                                                   *
 *                                                                                                          *
 * Get message size and set message size into fixed header                                                  *
 * @see http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf chapter 2.2.3                     *
 ************************************************************************************************************/
uint32_t set_size(MQTT_fixed_header_t * a_output_ptr,
                  size_t                a_message_size)
{
    uint8_t return_value = 0;
    if ((MQTT_MAX_MESSAGE_SIZE > a_message_size) && /* Message size in boundaries 0-max */
        (NULL != a_output_ptr))                     /* Output pointer is not NULL       */
    {
        /* Construct size from MQTT message - see the spec.*/
        do {
            uint8_t encodedByte = a_message_size % 128;
            a_message_size      = a_message_size / 128;

            if (a_message_size > 0)
                encodedByte = encodedByte | 128;

            a_output_ptr->length[return_value] = encodedByte;
            return_value++;
        } while (a_message_size > 0);

    } else {
        return 0;
    }
    return return_value;
}

uint8_t * get_size(uint8_t  * a_input_ptr,
                   uint32_t * a_message_size_ptr)
{
    uint32_t multiplier = 1;
    uint32_t value      = 0;
    uint8_t  cnt        = 1;
    uint8_t  aByte      = 0;

    /* Verify input parameters */
    if ((NULL == a_input_ptr) ||
        (NULL == a_message_size_ptr))
    {
        #ifdef DEBUG
            mqtt_printf("%s %u Invalid parameters %p %p\n",
                        __FILE__,
                        __LINE__,
                        a_input_ptr,
                        a_message_size_ptr);
        #endif
        return NULL;
    }

    /* Construct size from MQTT message - see the spec link above*/
    *a_message_size_ptr = 0;
    do {
        aByte = a_input_ptr[cnt++];
        value += ((aByte & 127) * multiplier);
        if (multiplier > (128*128*128)){
            #ifdef DEBUG
                mqtt_printf("Message size is too big %s %i %u\n", __FILE__, __LINE__, value);
            #endif
            return NULL;
        }
        multiplier *= 128;
    } while (0 != (aByte & 128));

    /* Verify that size is supported by applicaiton */
    if (MQTT_MAX_MESSAGE_SIZE < value) {
        #ifdef DEBUG
            mqtt_printf("%s %u Size is too big %u\n", __FILE__, __LINE__, value);
        #endif
        return NULL;
    }

    *a_message_size_ptr = value;
    return (a_input_ptr + cnt);
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection FixedHeader Fixed Header functions                                                           *
 *                                                                                                          *
 * Encode and decode fixed header functions                                                                 *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 2.2 Fixed header *
 *                                                                                                          *
 ************************************************************************************************************/
uint8_t encode_fixed_header(MQTT_fixed_header_t * a_output_ptr,
                            bool                  a_dup,
                            MQTTQoSLevel_t        a_qos,
                            bool                  a_retain,
                            MQTTMessageType_t     message_type,
                            uint32_t              message_size)
{
    if (NULL != a_output_ptr)
    {
        /* Test QoS boundaires and store it */
        if (QoSInvalid > a_qos) {
            a_output_ptr->flagsAndType.qos = a_qos;
        } else {
            #ifdef DEBUG
                mqtt_printf("%s %u Invalid QoS %i\n", __FILE__, __LINE__, a_qos);
            #endif
            return 0;
        }

        a_output_ptr->flagsAndType.dup    = a_dup;
        a_output_ptr->flagsAndType.retain = a_retain;

        /* Test message type boundaires and store it */
        if (MAXCMD > message_type) {
            a_output_ptr->flagsAndType.message_type = message_type;
        } else {
            #ifdef DEBUG
                mqtt_printf("%s %u Invalid message type %u\n",
                            __FILE__,
                            __LINE__,
                            message_type);
            #endif
            return 0;
        }

        /* Test message size boundaires and store it. */
        uint32_t msgLenSize = set_size(a_output_ptr, message_size);

        if (0 < msgLenSize)
            return (msgLenSize + 1);

    }
    /* Return failure */
    return 0;
}

uint8_t * decode_fixed_header(uint8_t           * a_input_ptr,
                              bool              * a_dup_ptr,
                              MQTTQoSLevel_t    * a_qos_ptr,
                              bool              * a_retain_ptr,
                              MQTTMessageType_t * a_message_type_ptr,
                              uint32_t          * a_message_size_ptr)
{
    uint8_t * return_ptr = NULL;

    /* Check input parameters against NULL pointers */
    if ((NULL != a_input_ptr)        &&
        (NULL != a_dup_ptr)          &&
        (NULL != a_qos_ptr)          &&
        (NULL != a_retain_ptr)       &&
        (NULL != a_message_type_ptr) &&
        (NULL != a_message_size_ptr))
    {
        *a_dup_ptr          = false;
        *a_qos_ptr          = 0;
        *a_retain_ptr       = false;
        *a_message_type_ptr = 0;
        *a_message_size_ptr = (uint32_t)(-1);

        MQTT_fixed_header_t * input_header = (MQTT_fixed_header_t*)a_input_ptr;

        /* Construct size from received message */
        return_ptr = get_size(a_input_ptr, a_message_size_ptr);

        /* Validate and store fixed header parameters */
        if ((NULL        != return_ptr)                              && /* Size was valid              */
            (QoSInvalid  != input_header->flagsAndType.qos)          && /* QoS parameter is valid      */
            (INVALIDCMD  <= input_header->flagsAndType.message_type) && /* Message type is in the rage */
            (MAXCMD       > input_header->flagsAndType.message_type)){

            /* Resut is valid, store parameters into function arguments */
            *a_dup_ptr          = input_header->flagsAndType.dup;
            *a_qos_ptr          = input_header->flagsAndType.qos;
            *a_retain_ptr       = input_header->flagsAndType.retain;
            *a_message_type_ptr = input_header->flagsAndType.message_type;

        } else {
            *a_message_size_ptr = 0; /* Clear message size */

            #ifdef DEBUG
                mqtt_printf("%s %u Invalid argument %x %x %x %u %x %p\n",
                            __FILE__,
                            __LINE__,
                            input_header->flagsAndType.dup,
                            input_header->flagsAndType.qos,
                            input_header->flagsAndType.retain,
                            *a_message_size_ptr,
                            input_header->flagsAndType.message_type,
                            a_input_ptr);
            #endif
        }
    }
    #ifdef DEBUG
        else {
            mqtt_printf("%s %u NULL argument given %p %p %p %p %p %p\n",
                        __FILE__,
                        __LINE__,
                        a_dup_ptr,
                        a_qos_ptr,
                        a_retain_ptr,
                        a_message_size_ptr,
                        a_message_type_ptr,
                        a_input_ptr);
        }
    #endif
    return return_ptr;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection EncodePublish Encode publish message                                                         *
 *                                                                                                          *
 * Form publish message from given parameters                                                               *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.3 PUBLISH      *
 *                                                                                                          *
 ************************************************************************************************************/
bool encode_publish(data_stream_out_fptr_t   a_out_fptr,
                    uint8_t                * a_output_ptr,
                    uint32_t                 a_output_size,
                    bool                     a_retain,
                    MQTTQoSLevel_t           a_qos,
                    bool                     a_dup,
                    uint8_t                * topic_ptr,
                    uint16_t                 topic_size,
                    uint16_t                 packet_identifier,
                    uint8_t                * message_ptr,
                    uint32_t                 message_size)
{
    bool ret = false;

    if ((NULL != a_output_ptr) &&
        (NULL != topic_ptr)    &&
        (NULL != message_ptr)  &&
        (sizeof(MQTT_fixed_header_t) < a_output_size)) { /* Buffer size is at least big enogh for header */

        uint32_t sizeOfMsg = message_size + topic_size + sizeof(uint16_t);

        if (a_qos > QoS0) /* If QoS set, then additional space is required */
            sizeOfMsg += sizeof(uint16_t);

        sizeOfMsg = encode_fixed_header((MQTT_fixed_header_t *) a_output_ptr,
                                                                a_retain,
                                                                a_qos,
                                                                a_dup,
                                                                PUBLISH,
                                                                sizeOfMsg);

        if ((0 < sizeOfMsg) &&
            (sizeOfMsg < a_output_size)) { /* Output buffer is big enough */

            /* First 2 bytes are topic_size */
            a_output_ptr[sizeOfMsg++] = ((topic_size >> 8) & 0xFF);
            a_output_ptr[sizeOfMsg++] = ((topic_size >> 0) & 0xFF);

            /* Copy topic name */
            mqtt_memcpy((void*)&(a_output_ptr[sizeOfMsg]), topic_ptr, topic_size);
            sizeOfMsg += topic_size;

            if (a_qos > QoS0) {
                /* Copy packet identifier - valid only in QoS 1 and 2 levels */
                a_output_ptr[sizeOfMsg++] = (uint8_t)((packet_identifier >> 8) & 0xFF);
                a_output_ptr[sizeOfMsg++] = (uint8_t)((packet_identifier >> 0) & 0xFF);
            }

            /* Copy message of topic */
            mqtt_memcpy((void*)&(a_output_ptr[sizeOfMsg]), message_ptr, message_size);
            sizeOfMsg +=message_size;

            // Send CONNECT message to the broker without flags
            if (a_out_fptr(a_output_ptr, sizeOfMsg) == (int)sizeOfMsg)
                ret = true;
            #ifdef DEBUG
                else
                    mqtt_printf("%s %u Sending publish failed %u",
                                __FILE__,
                                __LINE__,
                                sizeOfMsg);
            #endif
        }
        #ifdef DEBUG
            else {
                mqtt_printf("%s %u Fixed header failed ",
                            __FILE__,
                            __LINE__);
            }
        #endif
    }
    #ifdef DEBUG
        else {
            mqtt_printf("%s %u Invalid argument given %p %s\n",
                        __FILE__,
                        __LINE__,
                        a_output_ptr,
                        topic_ptr);
        }
    #endif
    return ret;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection DecodePublish Decode publish message                                                         *
 *                                                                                                          *
 * Decode publish message from from input stream starting from variable header.                             *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.3 PUBLISH      *
 *                                                                                                          *
 ************************************************************************************************************/
bool decode_publish(uint8_t         * a_message_in_ptr,
                    uint32_t          a_size_of_msg,
                    MQTTQoSLevel_t    a_qos,
                    uint8_t        ** a_topic_out_ptr,
                    uint16_t        * a_topic_length_out_ptr,
                    uint8_t        ** a_out_message_ptr,
                    uint32_t        * a_out_message_size_ptr)
{
    bool ret = false;

    if ((NULL != a_message_in_ptr)       &&
        (NULL != a_topic_out_ptr)        &&
        (NULL != a_topic_length_out_ptr)) {

        /* Decode variable header = topic name and length - read topic out and get pointer to payload */
        uint8_t * payload = decode_variable_header_publish(a_message_in_ptr,
                                                           a_topic_out_ptr,
                                                           a_qos,
                                                           a_topic_length_out_ptr);

        if ((NULL != payload)                   &&
            (NULL != *a_topic_out_ptr)          &&
            (NULL != a_out_message_size_ptr)    &&
            (0     < (*a_topic_length_out_ptr)) &&
            (NULL != a_out_message_ptr)) {

            /* Count ant store message size */
            uint32_t header_size = (payload - a_message_in_ptr);

            if (header_size < a_size_of_msg) {
                *a_out_message_size_ptr = a_size_of_msg - header_size;
                *a_out_message_ptr      = payload;
                ret = true;
            }
            #ifdef DEBUG
                else {
                    mqtt_printf("%s %u header size is bigger than reserved message size %u > %u\n",
                                __FILE__,
                                __LINE__,
                                header_size,
                                a_size_of_msg);
                }
            #endif
        }
        #ifdef DEBUG
            else {
                mqtt_printf("%s %u variable decode failed\n",
                            __FILE__,
                            __LINE__);
            }
        #endif
    }
    #ifdef DEBUG
        else {
            mqtt_printf("%s %u Invalid argument given %p, %p, %p, %p\n",
                        __FILE__,
                        __LINE__,
                        a_message_in_ptr,
                        a_message_in_ptr,
                        a_topic_out_ptr,
                        a_topic_length_out_ptr);
        }
    #endif
    return ret;
}

uint8_t * decode_variable_header_publish(uint8_t         * a_input_ptr,
                                         uint8_t        ** a_topic_out_ptr,
                                         MQTTQoSLevel_t    a_qos,
                                         uint16_t        * a_topic_length_out_ptr)
{
    uint8_t * next_hdr = NULL;

    if ((NULL != a_input_ptr)            &&
        (NULL != a_topic_length_out_ptr) &&
        (NULL != a_topic_out_ptr)) {

        uint32_t index = 0;
        /* First 2 bytes are topic_size */
        *a_topic_length_out_ptr  = (((uint16_t)(a_input_ptr[index++]) << 8) & 0xFF00); /* Higer byte */
        *a_topic_length_out_ptr |= (((uint16_t)(a_input_ptr[index++]) << 0) & 0x00FF); /* Lower byte */

        printf("topic length %u\n", *a_topic_length_out_ptr);

        /* Set pointer to point beginning of topic - no copy, reuse existing buffer. */
        *a_topic_out_ptr = &(a_input_ptr[index++]);

        index += *a_topic_length_out_ptr;

        if (a_qos > QoS0)
            /* Ignore 2 bytes packet identifiers - valid only in QoS 1 and 2 levels */
            index += 2;

        /* Set pointer to beginning of next header */
        next_hdr = (uint8_t*)&(a_input_ptr[index-1]);
    } else {
        #ifdef DEBUG
            mqtt_printf("%s %u NULL argument given %p\n",
                        __FILE__,
                        __LINE__,
                        a_input_ptr);
        #endif
        return NULL;
    }
    return next_hdr;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection EncodeSubscribe Encode subscribe message                                                     *
 *                                                                                                          *
 * Form subscribe message from given input parameters.                                                      *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.8 SUBSCRIBE    *
 *                                                                                                          *
 ************************************************************************************************************/
bool encode_subscribe(data_stream_out_fptr_t   a_out_fptr,
                      uint8_t                * a_output_ptr,
                      uint32_t                 a_output_size,
                      MQTTQoSLevel_t           a_topic_qos,
                      uint8_t                * a_topic_ptr,
                      uint16_t                 a_topic_size,
                      uint16_t                 a_packet_identifier)
{
    bool ret = false;

    if ((NULL != a_out_fptr)   &&
        (NULL != a_output_ptr) &&
        (NULL != a_topic_ptr)  &&
        (a_topic_size < a_output_size)) {

        /* topic string size + topic QoS + topic length + packet identifier. */
        uint32_t sizeOfMsg =  a_topic_size + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);

        if (a_topic_qos > QoS0) /* If QoS > 0, then next 2 bytes are packet identifier numbers. */
            sizeOfMsg += sizeof(uint16_t);

        /* In subscribe QoS must be 1 and rest remain zero */
        sizeOfMsg = encode_fixed_header((MQTT_fixed_header_t *) a_output_ptr,
                                        true,
                                        QoS0,
                                        false,
                                        SUBSCRIBE,
                                        sizeOfMsg);

        /* IF fixed header encode succeeded then parse reset of the message. */
        if ((0 < sizeOfMsg) &&
            (sizeOfMsg < a_output_size)) {

            if (1 /*a_topic_qos > QoS0*/) {
                /* Copy packet identifier - valid only in QoS 1 and 2 levels */
                a_output_ptr[sizeOfMsg++] = (uint8_t)((a_packet_identifier >> 8) & 0xFF);
                a_output_ptr[sizeOfMsg++] = (uint8_t)((a_packet_identifier >> 0) & 0xFF);
            }

            /* Copy topic length */
            a_output_ptr[sizeOfMsg++] = (uint8_t)((a_topic_size >> 8) & 0xFF);
            a_output_ptr[sizeOfMsg++] = (uint8_t)((a_topic_size >> 0) & 0xFF);

            /* Copy topic name */
            mqtt_memcpy((void*)&(a_output_ptr[sizeOfMsg]), a_topic_ptr, a_topic_size);
            sizeOfMsg += a_topic_size;

            /* QoS for subscribe */
            a_output_ptr[sizeOfMsg++] = a_topic_qos;

            /* Send SUBSCRIBE message */
            if (a_out_fptr(a_output_ptr, sizeOfMsg) == (int)sizeOfMsg)
                ret = true;
            #ifdef DEBUG
                else
                    mqtt_printf("%s %u Sending SUBSCRIBE failed %u,\n",
                                __FILE__,
                                __LINE__,
                                sizeOfMsg);
            #endif
        }
        #ifdef DEBUG
            else {
                mqtt_printf("%s %u Fixed header failed\n",
                            __FILE__,
                            __LINE__);
            }
        #endif
    }
    #ifdef DEBUG
        else {
            mqtt_printf("%s %u Invalid argument given %p %s\n",
                        __FILE__,
                        __LINE__,
                        a_output_ptr,
                        a_topic_ptr);
        }
    #endif
    return ret;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection DecodeSuback Decode suback message                                                           *
 *                                                                                                          *
 * Decode suback message from input stream.                                                                 *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.9 SUBACK       *
 *                                                                                                          *
 ************************************************************************************************************/
void decode_variable_header_suback(uint8_t          * a_input_ptr,
                                   MQTTErrorCodes_t * a_subscribe_state_ptr)
{
    if (NULL != a_subscribe_state_ptr) {
        *a_subscribe_state_ptr = -1;
        if (NULL != a_input_ptr)
        {
            *a_subscribe_state_ptr = *(a_input_ptr + 4); /*5th byte contains return value  */
        }
        #ifdef DEBUG
            else {
                mqtt_printf("%s %u NULL argument given %p\n",
                            __FILE__,
                            __LINE__,
                            a_input_ptr);
            }
        #endif
        *a_subscribe_state_ptr = (*a_subscribe_state_ptr == 0x80); /* 0x80 = successfull */
    }
    #ifdef DEBUG
        else {
            mqtt_printf("%s %u NULL argument given %p\n",
                        __FILE__,
                        __LINE__,
                        a_subscribe_state_ptr);
        }
    #endif
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection FillPayload Add payload into MQTT message                                                    *
 *                                                                                                          *
 * Copy payload into output buffer's payload loacation                                                      *
 *                                                                                                          *
 ************************************************************************************************************/
uint8_t * mqtt_add_payload_parameters(uint8_t * a_output_ptr,
                                      uint16_t  a_length,
                                      uint8_t * a_parameter_ptr)
{
    if ((NULL != a_output_ptr) &&
        (NULL != a_parameter_ptr)) {

        /* behavioral tested by returning NULL from here */

        /* Payload parameters conists of 2 byte length followed by the parameter */
        *a_output_ptr = (uint8_t)((a_length >> 8) & 0xFF);
        a_output_ptr++;

        *a_output_ptr = (uint8_t)((a_length >> 0) & 0xFF);
        a_output_ptr++;

        /* Copy actual data */
        mqtt_memcpy(a_output_ptr, a_parameter_ptr, a_length);

        return a_output_ptr + a_length;
    }
    #ifdef DEBUG
        mqtt_printf("%s %u NULL argument given %p %p\n",
                    __FILE__,
                    __LINE__,
                    a_output_ptr,
                    a_parameter_ptr);
    #endif
    return NULL;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection Connect Construct connect message                                                            *
 *                                                                                                          *
 * Decode connect message from given parameters into given buffer.                                          *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.1 CONNECT      *
 *                                                                                                          *
 ************************************************************************************************************/
MQTTErrorCodes_t mqtt_connect_(uint8_t                * a_message_buffer_ptr,
                               size_t                   a_max_buffer_size,
                               data_stream_in_fptr_t    a_in_fptr,
                               data_stream_out_fptr_t   a_out_fptr,
                               MQTT_connect_t         * a_connect_ptr,
                               bool                     wait_and_parse_response)
{
    /* Ensure that pointers are valid */
    if (NULL != a_out_fptr) {

        if ((NULL != a_message_buffer_ptr) &&
            (NULL != a_connect_ptr)) {

            uint16_t msg_size = 0;
            uint8_t * msg_ptr = mqtt_connect_fill(a_message_buffer_ptr,
                                                  a_max_buffer_size,
                                                  a_connect_ptr,
                                                  &msg_size);
            if (NULL != msg_ptr) {
                // Send CONNECT message to the broker without flags
                if (a_out_fptr(msg_ptr, msg_size) == (int)msg_size)
                {
                    if ((NULL != a_in_fptr) &&
                        (true == wait_and_parse_response))
                    {
                        // Wait response from borker
                        int rcv = a_in_fptr(a_message_buffer_ptr, a_max_buffer_size);
                        if (0 < rcv) {
                            return mqtt_connect_parse_ack(a_message_buffer_ptr);
                        } else
                        {
                            return ServerUnavailabe;
                        }
                    } else {
                        return Successfull;
                    }
                } else {
                    return ServerUnavailabe;
                }
            }
            #ifdef DEBUG
                else {
                    mqtt_printf("%s %u mqtt_connect_fill failed\n",
                                __FILE__,
                                __LINE__);
                }
            #endif
        }
    }
    return InvalidArgument;
}

uint8_t encode_variable_header_connect(uint8_t        * a_output_ptr,
                                       bool             a_clean_session,
                                       bool             a_last_will,
                                       MQTTQoSLevel_t   a_last_will_qos,
                                       bool             a_permanent_last_will,
                                       bool             a_password,
                                       bool             a_username,
                                       uint16_t         a_keepalive)
{

    uint8_t variable_header_size = 0;
    if ((NULL       != a_output_ptr) &&
        (QoSInvalid  > a_last_will_qos))
    {
        MQTT_variable_header_connect_t * header_ptr = (MQTT_variable_header_connect_t*) a_output_ptr;
        header_ptr->length[0]        = 0x00;
        header_ptr->length[1]        = 0x04; /* length of protocol name which is const => length is const */
        header_ptr->procol_name[0]   = 'M';
        header_ptr->procol_name[1]   = 'Q';
        header_ptr->procol_name[2]   = 'T';
        header_ptr->procol_name[3]   = 'T';
        header_ptr->protocol_version = 0x04; /* 0x04 = 3.1.1 version */
        header_ptr->keepalive[0]     = (uint8_t)((a_keepalive >> 8) & 0xFF);
        header_ptr->keepalive[1]     = (uint8_t)((a_keepalive >> 0) & 0xFF);

        /* Fill in connection flags */
        MQTT_variable_header_connect_flags_t * header_flags_ptr = (MQTT_variable_header_connect_flags_t*)&(header_ptr->flags);
        header_flags_ptr->reserved       = 0;
        header_flags_ptr->clean_session  = a_clean_session;
        header_flags_ptr->last_will      = a_last_will;
        header_flags_ptr->last_will_qos  = a_last_will_qos;
        header_flags_ptr->permanent_will = a_permanent_last_will;
        header_flags_ptr->password       = a_password;
        header_flags_ptr->username       = a_username;
        variable_header_size             = 10; /* fixed size */
    }
    #ifdef DEBUG
        else {
            mqtt_printf("%s %u Invalid argument given %p %x\n",
                        __FILE__,
                        __LINE__,
                        a_output_ptr,
                        a_last_will_qos);
        }
    #endif
    return variable_header_size;
}

uint8_t * mqtt_connect_fill_a_param(uint8_t  * a_input_argument_str_ptr,
                                    bool       a_input_argument_is_mandatory,
                                    uint8_t  * a_output_ptr,
                                    int32_t  * a_remaining_size_ptr,
                                    uint16_t * a_ouput_size_ptr)
{
    if ((NULL != a_input_argument_str_ptr) &&
        (NULL != a_output_ptr)             &&
        (NULL != a_remaining_size_ptr)) {

        /* Fill client ID into payload. It must exists and it must be first parameter */
        uint16_t arg_len       = (uint16_t)mqtt_strlen((char*)(a_input_argument_str_ptr));
        *a_remaining_size_ptr -= arg_len;

        if (0 >= *a_remaining_size_ptr) {
            #ifdef DEBUG
                mqtt_printf("%s %u Not enough space %i\n",
                            __FILE__,
                            __LINE__,
                            *a_remaining_size_ptr);
            #endif
            return NULL;
        }

        if (0 < arg_len) {
            return mqtt_add_payload_parameters(a_output_ptr,
                                               arg_len,
                                               a_input_argument_str_ptr);
        } else if (a_input_argument_is_mandatory) {
            *a_ouput_size_ptr = 0;
            #ifdef DEBUG
                mqtt_printf("%s %u Required parameter not set %s\n",
                    __FILE__,
                    __LINE__,
                    a_input_argument_str_ptr);
            #endif
        }
    } else {
        #ifdef DEBUG
            mqtt_printf("%s %u NULL argument given %p %p %p\n",
                        __FILE__,
                        __LINE__,
                        a_input_argument_str_ptr,
                        a_output_ptr,
                        a_remaining_size_ptr);
        #endif
    }
    return NULL;
}

/* Client ID, last will, username and password */
uint8_t * mqtt_connect_fill_parameters(uint8_t        * a_message_buffer_ptr,
                                       MQTT_connect_t * a_connect_ptr,
                                       uint16_t       * a_ouput_size_ptr,
                                       int32_t        * a_space_remaining_ptr)
{
    if ((NULL == a_message_buffer_ptr) ||
        (NULL == a_connect_ptr)        ||
        (NULL == a_ouput_size_ptr)     ||
        (NULL == a_space_remaining_ptr)) {
        #ifdef DEBUG
            mqtt_printf("%s %u NULL argument given %p %p %p %p\n",
            __FILE__,
            __LINE__,
            a_message_buffer_ptr,
            a_connect_ptr,
            a_ouput_size_ptr,
            a_space_remaining_ptr);
        #endif
        return NULL;
    }

    uint8_t * payload_ptr = a_message_buffer_ptr + sizeof(MQTT_fixed_header_t) + sizeof(MQTT_variable_header_connect_t);

    /* Client ID */
    payload_ptr = mqtt_connect_fill_a_param(a_connect_ptr->client_id,
                                            true,
                                            payload_ptr,
                                            a_space_remaining_ptr,
                                            a_ouput_size_ptr);
    if (NULL == payload_ptr)
        return NULL;

    /* Last will and testament */
    a_connect_ptr->connect_flags.last_will_qos = QoS0;
    if ((0 < mqtt_strlen((char*)(a_connect_ptr->last_will_topic))) &&
        (0 < mqtt_strlen((char*)(a_connect_ptr->last_will_message)))) {

        a_connect_ptr->connect_flags.last_will     = true;

        payload_ptr = mqtt_connect_fill_a_param(a_connect_ptr->last_will_topic,
                                                false,
                                                payload_ptr,
                                                a_space_remaining_ptr,
                                                a_ouput_size_ptr);
        if (NULL == payload_ptr)
            return NULL;

        payload_ptr = mqtt_connect_fill_a_param(a_connect_ptr->last_will_message,
                                                false,
                                                payload_ptr,
                                                a_space_remaining_ptr,
                                                a_ouput_size_ptr);

        if (NULL == payload_ptr)
            return NULL;

    } else {
        /* if not defined, disable will flag */
        a_connect_ptr->connect_flags.last_will     = false;
        a_connect_ptr->connect_flags.last_will_qos = QoS0;
    }

    /* Username */
    if (0 < mqtt_strlen((char*)(a_connect_ptr->username))) {

        a_connect_ptr->connect_flags.username = true;
        payload_ptr = mqtt_connect_fill_a_param(a_connect_ptr->username,
                                                false,
                                                payload_ptr,
                                                a_space_remaining_ptr,
                                                a_ouput_size_ptr);
        if (NULL == payload_ptr)
            return NULL;
    } else {
        a_connect_ptr->connect_flags.username = false;
    }

    /* Password */
    if (0 < mqtt_strlen((char*)(a_connect_ptr->password))) {

        a_connect_ptr->connect_flags.password = true;
        payload_ptr = mqtt_connect_fill_a_param(a_connect_ptr->password,
                                                false,
                                                payload_ptr,
                                                a_space_remaining_ptr,
                                                a_ouput_size_ptr);
        if (NULL == payload_ptr)
            return NULL;
    } else {
        a_connect_ptr->connect_flags.password = false;
    }

    return payload_ptr;
}

uint8_t * mqtt_connect_fill(uint8_t        * a_message_buffer_ptr,
                            size_t           a_max_buffer_size,
                            MQTT_connect_t * a_connect_ptr,
                            uint16_t       * a_ouput_size_ptr)
{
    uint8_t sizeOfVarHdr, sizeOfFixedHdr = 0;

    if ((NULL == a_message_buffer_ptr) ||
        (NULL == a_connect_ptr)        ||
        (NULL == a_ouput_size_ptr)) {
        #ifdef DEBUG
            mqtt_printf("%s %u NULL argument given %p %p %p\n",
            __FILE__,
            __LINE__,
            a_message_buffer_ptr,
            a_connect_ptr,
            a_ouput_size_ptr);
        #endif
        return NULL;
    }

    int32_t space_remaining = (int32_t)a_max_buffer_size;

    /* Clear message buffer */
    mqtt_memset(a_message_buffer_ptr, 0, a_max_buffer_size);

    /* Fill parameters and related flasgs */
    uint8_t * payload_ptr = mqtt_connect_fill_parameters(a_message_buffer_ptr,
                                                         a_connect_ptr,
                                                         a_ouput_size_ptr,
                                                         &space_remaining);
    if (NULL == payload_ptr)
        return NULL;

    /* Construct variable header with given parameters */
    sizeOfVarHdr = encode_variable_header_connect(a_message_buffer_ptr + sizeof(MQTT_fixed_header_t),
                                                  a_connect_ptr->connect_flags.clean_session,
                                                  a_connect_ptr->connect_flags.last_will,
                                                  a_connect_ptr->connect_flags.last_will_qos,
                                                  a_connect_ptr->connect_flags.permanent_will,
                                                  a_connect_ptr->connect_flags.password,
                                                  a_connect_ptr->connect_flags.username,
                                                  a_connect_ptr->keepalive);

    space_remaining -= sizeOfVarHdr;
    if (0 > space_remaining) { /* Not enough space */
        #ifdef DEBUG
            mqtt_printf("%s %u Not enough space", __FILE__, __LINE__);
        #endif
        return NULL;
    }


    uint32_t payloadSize = payload_ptr - (a_message_buffer_ptr + sizeof(MQTT_fixed_header_t) + sizeof(MQTT_variable_header_connect_t));

    space_remaining -= payloadSize;
    if (0 > space_remaining) { /* Not enough space */
        #ifdef DEBUG
            mqtt_printf("%s %u Not enough space", __FILE__, __LINE__);
        #endif
        return NULL;
    }


    MQTT_fixed_header_t temporaryFixedHeader;
    /* Form fixed header for CONNECT msg */
    sizeOfFixedHdr = encode_fixed_header(&temporaryFixedHeader,
                                         false,
                                         QoS0,
                                         false,
                                         CONNECT,
                                         payloadSize + sizeOfVarHdr);

    /* Count valid starting point for the message, because fixed header length variates between 2 to 4 bytes */
    uint8_t * message_ptr = a_message_buffer_ptr + (sizeof(MQTT_fixed_header_t) - sizeOfFixedHdr);

    /* Copy fixed header at the beginning of the message */
    mqtt_memcpy((void*)message_ptr, (void*)&temporaryFixedHeader, sizeOfFixedHdr);

    *a_ouput_size_ptr = (payload_ptr - message_ptr);

    return message_ptr;
}


/************************************************************************************************************
 *                                                                                                          *
 * \subsection DecodeConnAck Decode connack message                                                         *
 *                                                                                                          *
 * Decode connack message from input stream.                                                                *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.2 CONNACK      *
 *                                                                                                          *
 ************************************************************************************************************/
uint8_t * decode_variable_header_conack(uint8_t * a_input_ptr,
                                        uint8_t * a_connection_state_ptr)
{
    if ((NULL != a_connection_state_ptr) &&
        (NULL != a_input_ptr)) {
        *a_connection_state_ptr = *(a_input_ptr + 1); /* 2nd byte contains return value */
        return (a_input_ptr + 2); /* CONNACK is always 2 bytes long. */
    }
    #ifdef DEBUG
        mqtt_printf("%s %u NULL argument given %p %p\n",
                    __FILE__,
                    __LINE__,
                    a_connection_state_ptr,
                    a_input_ptr);
    #endif
    return NULL;
}

MQTTErrorCodes_t mqtt_connect_parse_ack(uint8_t * a_message_in_ptr)
{
    uint8_t connection_state = (uint8_t)InvalidArgument;
    if (NULL != a_message_in_ptr) {

        bool              dup, retain;
        MQTTQoSLevel_t    qos;
        MQTTMessageType_t type;
        uint32_t          size;

        // Decode fixed header
        uint8_t * nextHdr = decode_fixed_header(a_message_in_ptr, &dup, &qos, &retain, &type, &size);

        if ((NULL != nextHdr) &&
            (CONNACK == type)) {
            // Decode variable header
            decode_variable_header_conack(nextHdr, &connection_state);
        }
    }
    return connection_state;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection DisConnect Construct disconnect message                                                      *
 *                                                                                                          *
 * Form and send fixed header with DISCONNECT command ID.                                                   *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.1 CONNECT      *
 *                                                                                                          *
 ************************************************************************************************************/
MQTTErrorCodes_t mqtt_disconnect_(data_stream_out_fptr_t a_out_fptr)
{
    if (NULL != a_out_fptr) {
        /* Form and send fixed header with DISCONNECT command ID.
           Message is stored into temporary local buffer. */
        MQTT_fixed_header_t temporaryBuffer;
        uint8_t sizeOfFixedHdr = encode_fixed_header(&temporaryBuffer, false, QoS0, false, DISCONNECT, 0);

        /* Send disconnect message out */
        if (a_out_fptr((uint8_t*)&temporaryBuffer, sizeOfFixedHdr) == sizeOfFixedHdr)
            return Successfull;
        else
            return ServerUnavailabe;
    }
    return InvalidArgument;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection PingReq Construct ping request                                                               *
 *                                                                                                          *
 * Form and send ping requestst message.                                                                    *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.12 PINGREQ     *
 *                                                                                                          *
 ************************************************************************************************************/
MQTTErrorCodes_t mqtt_ping_req(data_stream_out_fptr_t a_out_fptr)
{
    if (NULL != a_out_fptr) {
        // Form and send fixed header with PINGREQ command ID
        MQTT_fixed_header_t temporaryBuffer;
        uint8_t sizeOfFixedHdr = encode_fixed_header(&temporaryBuffer, false, QoS0, false, PINGREQ, 0);

        if (a_out_fptr((uint8_t*)&temporaryBuffer, sizeOfFixedHdr) == sizeOfFixedHdr)
            return Successfull;
        else
            return ServerUnavailabe;
    }
    return InvalidArgument;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection PingResp Decode ping resp                                                                    *
 *                                                                                                          *
 * Decode ping resp message from given input stream.                                                        *
 * See <a href="http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.pdf">Chapter 3.13 PINGRESP    *
 *                                                                                                          *
 ************************************************************************************************************/
MQTTErrorCodes_t mqtt_parse_ping_ack(uint8_t * a_message_in_ptr)
{
    if (NULL != a_message_in_ptr) {

        bool              dup, retain;
        MQTTQoSLevel_t    qos;
        MQTTMessageType_t type;
        uint32_t          size;

        /* Decode fixed header */
        uint8_t * nextHdr = decode_fixed_header(a_message_in_ptr, &dup, &qos, &retain, &type, &size);

        /* If decoded successfully ...*/
        if ((NULL     != nextHdr) &&
            (PINGRESP == type)) {
            /* Decode variable header */
            return Successfull;
        }
    }
    return ServerUnavailabe;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection ParsInput Parse input stream                                                                 *
 *                                                                                                          *
 * Function decode fixed header from input stream to find out details of received message.                  *
 * Appropriate funciton is called to decode received message successfully.                                  *
 *                                                                                                          *
 ************************************************************************************************************/
MQTTErrorCodes_t mqtt_parse_input_stream(uint8_t  * a_input_ptr,
                                         uint32_t * a_message_size_ptr)
{
    MQTTErrorCodes_t  status = InvalidArgument;
    bool              dup, retain;
    MQTTQoSLevel_t    qos;
    MQTTMessageType_t type;

    if (NULL == a_input_ptr)
        return InvalidArgument;

    /* Decode fixed header */
    uint8_t * next_header_ptr = decode_fixed_header(a_input_ptr, &dup, &qos, &retain, &type, a_message_size_ptr);

    if (NULL == next_header_ptr)
        return InvalidArgument;

    /* Check message type to and take appropriate action. */
    switch (type)
    {
        case CONNACK:
            {
                uint8_t connection_state;
                if (NULL != decode_variable_header_conack(next_header_ptr, &connection_state)) {

                    if (Successfull == connection_state) {
                        g_shared_data->state = STATE_CONNECTED;
                        status = Successfull;

                    } else {
                        g_shared_data->state = STATE_DISCONNECTED;
                        status = Successfull;
                    }

                    if (NULL != g_shared_data->connected_cb_fptr)
                        g_shared_data->connected_cb_fptr(connection_state);
                    #ifdef DEBUG
                        else
                            mqtt_printf("%s %u Connection callback is NULL\n", __FILE__, __LINE__);
                    #endif
                }
                #ifdef DEBUG
                    else
                        mqtt_printf("%s %u decode_variable_header_conack returned NULL\n",
                                    __FILE__,
                                    __LINE__);
                #endif
            }
            break;

        case PUBLISH:
            {
                uint8_t * topic_ptr    = NULL;
                uint16_t  topic_length = 0;
                uint8_t * message_ptr  = NULL;
                uint32_t  message_size = 0;

                if (decode_publish(next_header_ptr,
                                   * a_message_size_ptr,
                                   qos,
                                   &topic_ptr,
                                   &topic_length,
                                   &message_ptr,
                                   &message_size)){

                    if (NULL != g_shared_data->subscribe_cb_fptr)
                        g_shared_data->subscribe_cb_fptr(Successfull,
                                                         message_ptr,
                                                         message_size,
                                                         topic_ptr,
                                                         topic_length);
                    #ifdef DEBUG
                        else
                            mqtt_printf("%s %u Subscribe callback is not set\n",
                                        __FILE__,
                                        __LINE__);
                    #endif
                    status = Successfull;
                } else {
                    if (NULL != g_shared_data->subscribe_cb_fptr)
                        g_shared_data->subscribe_cb_fptr(status, NULL, 0, NULL, 0);
                }
                break;
            }

        case SUBACK:
            {
				decode_variable_header_suback(a_input_ptr, &status);
				if (NULL != g_shared_data) {

					if (NULL != g_shared_data->subscribe_cb_fptr) {
						if (true == status) {
							g_shared_data->subscribe_cb_fptr(Successfull, NULL, 0, NULL, 0);
							status = Successfull;
						}
						else {
							g_shared_data->subscribe_cb_fptr(PublishDecodeError, NULL, 0, NULL, 0);
						}
					}
				}
            }
            break;

        case PINGRESP:
            status = Successfull;
            break;

        default:
            status = InvalidArgument;
            break;
    }
    return status;
}

/************************************************************************************************************
 *                                                                                                          *
 * \subsection API MQTT API functions                                                                       *
 *                                                                                                          *
 * External software components uses services of this library through these API functions.                  *
 *                                                                                                          *
 ************************************************************************************************************/
MQTTErrorCodes_t mqtt(MQTTAction_t         a_action,
                      MQTT_action_data_t * a_action_ptr)
{
        MQTTErrorCodes_t status = InvalidArgument;

        switch (a_action)
        {
            case ACTION_INIT:
                if (NULL != a_action_ptr) {

                    g_shared_data                          = a_action_ptr->action_argument.shared_ptr;
                    g_shared_data->state                   = STATE_DISCONNECTED;
                    g_shared_data->mqtt_packet_cntr        = 0;
                    g_shared_data->keepalive_in_ms         = 0;
                    g_shared_data->time_to_next_ping_in_ms = 0;
                    status = Successfull;
                }
                break;

            case ACTION_DISCONNECT:
                if ((NULL               != g_shared_data) &&
                    (STATE_DISCONNECTED != g_shared_data->state))
                    status = mqtt_disconnect_(g_shared_data->out_fptr);
                else
                    status = NoConnection;
                break;

            case ACTION_CONNECT:
                if ((NULL != g_shared_data) &&
                    (NULL != a_action_ptr)) {
                    if (g_shared_data->state == STATE_DISCONNECTED) {
                        status = mqtt_connect_(g_shared_data->buffer,
                                               g_shared_data->buffer_size,
                                               NULL,
                                               g_shared_data->out_fptr,
                                               a_action_ptr->action_argument.connect_ptr,
                                               false);

                        if (Successfull == status) {

                            if (0 != a_action_ptr->action_argument.connect_ptr->keepalive) {
                                g_shared_data->keepalive_in_ms  = (a_action_ptr->action_argument.connect_ptr->keepalive) * 1000;
                                g_shared_data->keepalive_in_ms -= 500;
                            } else {
                                g_shared_data->keepalive_in_ms = INT32_MIN;
                            }
                            g_shared_data->time_to_next_ping_in_ms = 0; /* Send Ping immediatelly*/
                            g_shared_data->state = STATE_CONNECTED;
                        } else {
                            g_shared_data->state = STATE_DISCONNECTED;
                        }
                    } else {
                        status = AllreadyConnected;
                    }
                }
                break;

            case ACTION_PUBLISH:
                if ((NULL            != g_shared_data)        &&
                    (STATE_CONNECTED == g_shared_data->state) &&
                    (NULL            != a_action_ptr)) {

                        uint8_t * message_buffer      = g_shared_data->buffer;
                        uint32_t  message_buffer_size = g_shared_data->buffer_size;

                        /* Use special buffer, not the shared one */
                        if ((NULL != a_action_ptr->action_argument.publish_ptr->output_buffer_ptr) &&
                            (0     < a_action_ptr->action_argument.publish_ptr->output_buffer_size)){

                               message_buffer = a_action_ptr->action_argument.publish_ptr->output_buffer_ptr;
                               message_buffer_size = a_action_ptr->action_argument.publish_ptr->output_buffer_size;
                           }
                       if (true == encode_publish(g_shared_data->out_fptr,
                                                   message_buffer,
                                                   message_buffer_size,
                                                   a_action_ptr->action_argument.publish_ptr->flags.retain,
                                                   a_action_ptr->action_argument.publish_ptr->flags.qos,
                                                   false, /* a_action_ptr->action_argument.publish_ptr->flags.dup,*/
                                                   a_action_ptr->action_argument.publish_ptr->topic_ptr,
                                                   a_action_ptr->action_argument.publish_ptr->topic_length,
                                                   g_shared_data->mqtt_packet_cntr++,
                                                   a_action_ptr->action_argument.publish_ptr->message_buffer_ptr,
                                                   a_action_ptr->action_argument.publish_ptr->message_buffer_size)) {

                            g_shared_data->time_to_next_ping_in_ms = g_shared_data->keepalive_in_ms;
                            status = Successfull;
                        }
                        #ifdef DEBUG
                            else {
                               mqtt_printf("%s %u Publish encode failed\n", __FILE__, __LINE__);
                            }
                        #endif
                }
                break;

            case ACTION_SUBSCRIBE:

                if ((NULL            != g_shared_data) &&
                    (STATE_CONNECTED == g_shared_data->state) &&
                    (NULL            != a_action_ptr)) {

                        if (true == encode_subscribe(g_shared_data->out_fptr,
                                                     g_shared_data->buffer,
                                                     g_shared_data->buffer_size,
                                                     a_action_ptr->action_argument.subscribe_ptr->qos,
                                                     a_action_ptr->action_argument.subscribe_ptr->topic_ptr,
                                                     a_action_ptr->action_argument.subscribe_ptr->topic_length,
                                                     g_shared_data->mqtt_packet_cntr++)) {

                            g_shared_data->time_to_next_ping_in_ms = g_shared_data->keepalive_in_ms;
                            status = Successfull;
                            g_shared_data->subscribe_status = true;
                        }
                }
                break;

            case ACTION_KEEPALIVE:
                if (NULL != a_action_ptr) {
                    if (STATE_CONNECTED == g_shared_data->state) {

                        if (INT32_MIN != g_shared_data->keepalive_in_ms) {

                            if (g_shared_data->time_to_next_ping_in_ms  > (int32_t) a_action_ptr->action_argument.epalsed_time_in_ms)
                                g_shared_data->time_to_next_ping_in_ms -= (int32_t) a_action_ptr->action_argument.epalsed_time_in_ms;
                            else
                                g_shared_data->time_to_next_ping_in_ms = 0;

                            if ( 0 >= g_shared_data->time_to_next_ping_in_ms) {
                                status = mqtt_ping_req(g_shared_data->out_fptr);
                                if (Successfull == status)
                                    g_shared_data->time_to_next_ping_in_ms = g_shared_data->keepalive_in_ms;
                                #ifdef DBUG
                                    else
                                        mqtt_printf("%s %u keep alive failed %u\n", __FILE__, __LINE__, status);
                                #endif
                            } else {
                                status = PingNotSend;
                            }
                        } else {
                            status = Successfull;
                        }
                    }
                }
                #ifdef DEBUG
                    else {
                        mqtt_printf("%s %u Keepalive argument NULL %p\n", __FILE__, __LINE__, (void*)a_action_ptr);
                    }
                #endif
                break;

            case ACTION_PARSE_INPUT_STREAM:
                status = mqtt_parse_input_stream(a_action_ptr->action_argument.input_stream_ptr->data,
                                                 &(a_action_ptr->action_argument.input_stream_ptr->size_of_data));
                if (NULL != g_shared_data)
                     g_shared_data->time_to_next_ping_in_ms = g_shared_data->keepalive_in_ms;
                break;

            default:
                #ifdef DEBUG
                    mqtt_printf("%s %u Invalid MQTT command %u\n", __FILE__, __LINE__, (uint32_t)a_action);
                #endif
                status = InvalidArgument;
                break;
        }

    return status;
}

bool mqtt_connect(char                   * a_client_name_ptr,
                  uint16_t                 a_keepalive_timeout,
                  uint8_t                * a_username_str_ptr,
                  uint8_t                * a_password_str_ptr,
                  uint8_t                * a_last_will_topic_str_ptr,
                  uint8_t                * a_last_will_str_ptr,
                  MQTT_shared_data_t     * mqtt_shared_data_ptr,
                  uint8_t                * a_output_buffer_ptr,
                  size_t                   a_output_buffer_size,
                  bool                     a_clean_session,
                  data_stream_out_fptr_t   a_out_write_fptr,
                  connected_fptr_t         a_connected_fptr,
                  subscrbe_fptr_t          a_subscribe_fptr,
                  uint8_t                  a_timeout_in_sec)
{
    /* Initialize MQTT */
    if ((NULL != a_client_name_ptr)         &&
        (NULL != a_username_str_ptr)        &&
        (NULL != a_password_str_ptr)        &&
        (NULL != a_last_will_topic_str_ptr) &&
        (NULL != a_last_will_str_ptr)       &&
        (NULL != mqtt_shared_data_ptr)      &&
        (NULL != a_output_buffer_ptr)) {

        g_shared_data = mqtt_shared_data_ptr;
        g_shared_data->buffer             = a_output_buffer_ptr;
        g_shared_data->buffer_size        = a_output_buffer_size;

        g_shared_data->out_fptr           = a_out_write_fptr;
        g_shared_data->connected_cb_fptr  = a_connected_fptr;
        g_shared_data->subscribe_cb_fptr  = a_subscribe_fptr;

        MQTT_action_data_t action;
        action.action_argument.shared_ptr = g_shared_data;

        MQTTErrorCodes_t state = mqtt(ACTION_INIT,
                                      &action);

        /* Connect to broker */
        if (Successfull == state) {
            #ifdef DEBUG
                mqtt_printf("%s %u MQTT Initialized\n", __FILE__, __LINE__);
            #endif

            /* Connect to broker */
            MQTT_connect_t connect_params;
            connect_params.client_id                    = (uint8_t *)a_client_name_ptr;
            connect_params.last_will_topic              = a_last_will_topic_str_ptr;
            connect_params.last_will_message            = a_last_will_str_ptr;
            connect_params.connect_flags.last_will_qos  = QoS0;
            connect_params.connect_flags.permanent_will = false;
            connect_params.username                     = a_username_str_ptr;
            connect_params.password                     = a_password_str_ptr;
            connect_params.keepalive                    = a_keepalive_timeout;
            connect_params.connect_flags.clean_session  = a_clean_session;

            action.action_argument.connect_ptr = &connect_params;

            state = mqtt(ACTION_CONNECT,
                         &action);

            if (Successfull == state) {

                uint8_t timeout = (a_timeout_in_sec * 10);

                while ((0!= timeout) &&
                       (STATE_CONNECTED != g_shared_data->state)) {
                    timeout--;
                    mqtt_sleep(0.1);
                }
            }
        }
    }

    return (g_shared_data->state == STATE_CONNECTED);
}

bool mqtt_disconnect()
{
    return (Successfull == mqtt(ACTION_DISCONNECT, NULL));
}

bool mqtt_publish(char * a_topic_ptr,
                  size_t a_topic_size,
                  char * a_msg_ptr,
                  size_t a_msg_size)
{
    /* Use internal shared buffer for sending */
    return mqtt_publish_buf(a_topic_ptr,
                            a_topic_size,
                            a_msg_ptr,
                            a_msg_size,
                            NULL,
                            0);
}

bool mqtt_publish_buf(char    * a_topic_ptr,
                      size_t    a_topic_size,
                      char    * a_msg_ptr,
                      size_t    a_msg_size,
                      uint8_t * a_output_buffer_ptr,
                      uint32_t  a_output_buffer_size)
{
    if ((NULL != a_topic_ptr) &&
        (NULL != a_msg_ptr)) {
        /* a_output_buffer_ptr can be NULL, in that case shared buffer is used */

        MQTT_publish_t publish;
        publish.flags.dup           = false;
        publish.flags.retain        = false;
        publish.flags.qos           = QoS0;
        publish.topic_ptr           = (uint8_t*)a_topic_ptr;
        publish.topic_length        = (uint16_t)a_topic_size;
        publish.message_buffer_ptr  = (uint8_t*)a_msg_ptr;
        publish.message_buffer_size = a_msg_size;
        publish.output_buffer_ptr   = a_output_buffer_ptr;
        publish.output_buffer_size  = a_output_buffer_size;

        MQTT_action_data_t action;
        action.action_argument.publish_ptr = &publish;

        MQTTErrorCodes_t state = mqtt(ACTION_PUBLISH,
                                    &action);

        if (Successfull == state)
            return true;
    }
    return false;
}

bool mqtt_subscribe(char     * a_topic,
                    uint16_t   a_topic_size,
                    uint8_t    a_timeout_in_sec)
{
    MQTTErrorCodes_t state = InvalidArgument;

    if ((NULL != g_shared_data) &&
        (NULL != a_topic)       &&
        (0     < a_topic_size)) {

        MQTT_subscribe_t subscribe;
        subscribe.qos          = QoS0;
        subscribe.topic_ptr    = (uint8_t*) a_topic;
        subscribe.topic_length = a_topic_size;

        MQTT_action_data_t action;
        action.action_argument.subscribe_ptr = &subscribe;

        state = mqtt(ACTION_SUBSCRIBE, &action);

        if (Successfull == state) {
            /* Do not perform responce chek when timeout is set to zero. */
            if (0 < a_timeout_in_sec) {

                uint8_t timeout = (a_timeout_in_sec * 10);
                while ((0 != timeout) &&
                       (false == g_shared_data->subscribe_status)) {
                    timeout--;
                    mqtt_sleep(0.1);
                }

                if (true == g_shared_data->subscribe_status)
                    state = Successfull;
            }
        }
        g_shared_data->subscribe_status = false;
    }
    return (Successfull == state);
}

bool mqtt_keepalive(uint32_t a_duration_in_ms)
{
    MQTT_action_data_t ap;
    ap.action_argument.epalsed_time_in_ms = a_duration_in_ms;

    MQTTErrorCodes_t state = mqtt(ACTION_KEEPALIVE, &ap);
    return ((Successfull == state) ||
            (PingNotSend == state));
}

bool mqtt_receive(uint8_t * a_data, size_t a_amount)
{
    if (NULL != a_data)
    {
        /* Parse input messages */
        MQTT_input_stream_t input;

        input.data         = a_data;
        input.size_of_data = (uint32_t)a_amount;

        MQTT_action_data_t action;
        action.action_argument.input_stream_ptr = &input;

        return (Successfull == mqtt(ACTION_PARSE_INPUT_STREAM, &action));
    }

    return false;
}
