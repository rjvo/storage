#include "mqtt.h"
#include "unity.h"

/* Functions not declared in mqtt.h - internal functions */
extern uint8_t * get_size(uint8_t * a_input_ptr, size_t * a_message_size_ptr);
extern uint8_t * decode_fixed_header(uint8_t * a_input_ptr,
                                     bool * a_dup_ptr,
                                     MQTTQoSLevel_t * a_qos_ptr,
                                     bool * a_retain_ptr,
                                     MQTTMessageType_t * a_message_type_ptr,
                                     uint32_t * a_message_size_ptr);
/****************************************************************************************
 * Header size tests                                                                    *
 ****************************************************************************************/

void test_decode_fixed_header_size_tiny_min()
{
    uint8_t input[] = {0x00, 0x00, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT8(0, msgSize);
}

void test_decode_fixed_header_size_tiny_max()
{
    uint8_t input[] = {0x00, 0x7F, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(127, msgSize);
}

void test_decode_fixed_header_size_small_min()
{
    uint8_t input[] = {0x00, 0x80, 0x01, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[3]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(128, msgSize);
}

void test_decode_fixed_header_size_small_max()
{
    uint8_t input[] = {0x00, 0xFF, 0x7F, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[3]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(16383, msgSize);
}

void test_decode_fixed_header_size_big_min()
{
    uint8_t input[] = {0x00, 0x80, 0x80, 0x01, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[4]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(16384, msgSize);
}

void test_decode_fixed_header_size_big_max()
{
    uint8_t input[] = {0x00, 0xFF, 0xFF, 0x7F, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[4]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(2097151, msgSize);
}

void test_decode_fixed_header_size_large_min()
{
    uint8_t input[] = {0x00, 0x80, 0x80, 0x80, 0x01, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[5]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(2097152, msgSize);
}

void test_decode_fixed_header_size_large_max()
{
    uint8_t input[] = {0x00, 0xFF, 0xFF, 0xFF, 0x7F, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(&(input[5]), get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(268435455, msgSize);
}

void test_decode_fixed_header_size_too_big()
{
    uint8_t input[] = {0x00, 0xFF, 0xFF, 0xFF, 0x80, 0x01, 0x00};
    size_t msgSize  = 0;
    TEST_ASSERT_EQUAL_PTR(NULL, get_size(input, &msgSize));
    TEST_ASSERT_EQUAL_UINT32(0, msgSize);
}

/****************************************************************************************
 * FIXED HEADER TESTS                                                                   *
 ****************************************************************************************/
void test_decode_fixed_header_all_zeros()
{
    /* Thest that fixed header with all zeros is really 0x0000 */
    uint8_t input[]        = {0x00, 0x00, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xF;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_dub_set()
{
    uint8_t input[]        = {0x02, 0x00, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xF;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x01, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_qos1()
{
    uint8_t input[]        = {0x04, 0x00, 0x00};
    bool dup               = 0;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xF;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x01, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_qos2()
{
    uint8_t input[]        = {0x08, 0x00, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xF;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x02, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_retain_set()
{
    uint8_t input[]        = {0x01, 0x00, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 0;
    MQTTMessageType_t type = 0xF;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x01, retain);
}

void test_decode_fixed_header_with_first_command()
{
    uint8_t input[]        = {0x10, 0x00, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xF;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x01, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_last_command()
{
    uint8_t input[]        = {0xE0, 0x00, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xE;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x0E, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_tiny_message()
{
    uint8_t input[]        = {0x00, 0x7F, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xE;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[2]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x0000007F, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_small_message()
{
    uint8_t input[]        = {0x00, 0x82, 0x01, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xE;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[3]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000082, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_big_message()
{
    uint8_t input[]        = {0x00, 0x82, 0x80, 0x01, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xE;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[4]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00004002, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_large_message()
{
    uint8_t input[]        = {0x00, 0x82, 0x80, 0x80, 0x01, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xE;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(&(input[5]), decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00200002, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

void test_decode_fixed_header_with_too_big_message()
{
    uint8_t input[]        = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    bool dup               = 1;
    MQTTQoSLevel_t qos     = 2;
    bool retain            = 1;
    MQTTMessageType_t type = 0xE;
    uint32_t size          = 19;
    TEST_ASSERT_EQUAL_PTR(NULL, decode_fixed_header(input, &dup, &qos, &retain, &type, &size));
    TEST_ASSERT_EQUAL_HEX32(0x00000000, size);
    TEST_ASSERT_EQUAL_HEX8(0x00, dup);
    TEST_ASSERT_EQUAL_HEX8(0x00, qos);
    TEST_ASSERT_EQUAL_HEX8(0x00, type);
    TEST_ASSERT_EQUAL_HEX8(0x00, retain);
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("Decode fixed header");
    unsigned int tCntr = 1;

    /* Get size tests */
    RUN_TEST(test_decode_fixed_header_size_tiny_min,         tCntr++);
    RUN_TEST(test_decode_fixed_header_size_tiny_max,         tCntr++);
    RUN_TEST(test_decode_fixed_header_size_small_min,        tCntr++);
    RUN_TEST(test_decode_fixed_header_size_small_max,        tCntr++);
    RUN_TEST(test_decode_fixed_header_size_big_min,          tCntr++);
    RUN_TEST(test_decode_fixed_header_size_big_max,          tCntr++);
    RUN_TEST(test_decode_fixed_header_size_large_min,        tCntr++);
    RUN_TEST(test_decode_fixed_header_size_large_max,        tCntr++);
    RUN_TEST(test_decode_fixed_header_size_too_big,          tCntr++);

    /* Fixed header decode tests */
    RUN_TEST(test_decode_fixed_header_all_zeros,             tCntr++);
    RUN_TEST(test_decode_fixed_header_with_dub_set,          tCntr++);
    RUN_TEST(test_decode_fixed_header_with_qos1,             tCntr++);
    RUN_TEST(test_decode_fixed_header_with_qos2,             tCntr++);
    RUN_TEST(test_decode_fixed_header_with_retain_set,       tCntr++);
    RUN_TEST(test_decode_fixed_header_with_first_command,    tCntr++);
    RUN_TEST(test_decode_fixed_header_with_last_command,     tCntr++);
    RUN_TEST(test_decode_fixed_header_with_tiny_message,     tCntr++);
    RUN_TEST(test_decode_fixed_header_with_small_message,    tCntr++);
    RUN_TEST(test_decode_fixed_header_with_big_message,      tCntr++);
    RUN_TEST(test_decode_fixed_header_with_large_message,    tCntr++);
    RUN_TEST(test_decode_fixed_header_with_too_big_message,  tCntr++);
    return (UnityEnd());
}
