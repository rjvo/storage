#include "mqtt.h"
#include "unity.h"

/* Functions not declared in mqtt.h - internal functions */
extern uint8_t * decode_variable_header_conack(uint8_t * a_input_ptr, uint8_t * a_connection_state_ptr);
extern uint8_t encode_variable_header_connect(uint8_t * a_output_ptr, 
                                              bool a_clean_session,
                                              bool a_last_will,
                                              MQTTQoSLevel_t a_last_will_qos,
                                              bool a_permanent_last_will,
                                              bool a_password,
                                              bool a_username,
                                              uint16_t a_keepalive);

/****************************************************************************************
 * FIXED HEADER TESTS                                                                   *
 * CONNACK                                                                              *
 ****************************************************************************************/
void test_decode_variable_header_connack()
{
    /* Thest that fixed header with all zeros is really 0x0000 */
    uint8_t input[] = {0x00, 0x01, 0x00};
    uint8_t state = 2;
    TEST_ASSERT_EQUAL_PTR(input + 2, decode_variable_header_conack(input, &state));
    TEST_ASSERT_EQUAL_UINT8(0x01, state);
    input[1] = 0xFF;
    TEST_ASSERT_EQUAL_PTR(input + 2, decode_variable_header_conack(input, &state));
    TEST_ASSERT_EQUAL_UINT8(0xFF, state);
}

void test_decode_variable_header_connect_zero()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x00, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS0,
                                                               false,
                                                               false,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_user()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x80, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS0,
                                                               false,
                                                               false,
                                                               true,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_pass()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x40, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS0,
                                                               false,
                                                               true,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_perm_lwt()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x20, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS0,
                                                               true,
                                                               false,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_qos2()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x10, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS2,
                                                               false,
                                                               false,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_qos1()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x08, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS1,
                                                               false,
                                                               false,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_lwt()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x04, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               true, 
                                                               QoS0,
                                                               false,
                                                               false,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_flag_clean_session()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x00, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               true,
                                                               false, 
                                                               QoS0,
                                                               false,
                                                               false,
                                                               false,
                                                               0));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_keepalive_short()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x00, 0x00, 0x01};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               false,
                                                               false, 
                                                               QoS0,
                                                               false,
                                                               false,
                                                               false,
                                                               1));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

void test_decode_variable_header_connect_keepalive_long()
{
    uint8_t output[10];
    uint8_t expected[] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0x02, 0x20, 0x00};
    TEST_ASSERT_EQUAL_UINT8(10, encode_variable_header_connect(output,
                                                               true,
                                                               false, 
                                                               QoS0,
                                                               false,
                                                               false,
                                                               false,
                                                               0x2000));
    TEST_ASSERT_EQUAL_MEMORY_ARRAY(expected, output, 1, 10);
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{  
    UnityBegin("Decode variable header");
    unsigned int tCntr = 1;

    /* CONNACK */
    RUN_TEST(test_decode_variable_header_connack,                    tCntr++);

    /* Connect request frame */
    RUN_TEST(test_decode_variable_header_connect_zero,               tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_user,          tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_pass,          tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_perm_lwt,      tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_qos2,          tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_qos1,          tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_lwt,           tCntr++);
    RUN_TEST(test_decode_variable_header_connect_flag_clean_session, tCntr++);
    RUN_TEST(test_decode_variable_header_connect_keepalive_short,    tCntr++);
    RUN_TEST(test_decode_variable_header_connect_keepalive_long,     tCntr++);

    return (UnityEnd());
}
