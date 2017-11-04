#include "mqtt.h"
#include "unity.h"

#define TO_HEX_16(_a_) (*(uint16_t*)&_a_)

/* Functions not declared in mqtt.h - internal functions */
extern uint8_t set_size(MQTT_fixed_header_t * a_output_ptr, size_t a_message_size);
extern uint8_t encode_fixed_header(MQTT_fixed_header_t * output,
                                   bool dup,
                                   MQTTQoSLevel_t qos,
                                   bool retain,
                                   MQTTMessageType_t messageType,
                                   uint32_t msgSize);

/****************************************************************************************
 * Header size tests                                                                    *
 ****************************************************************************************/

void test_encode_fixed_header_size(uint8_t expected_return_value, size_t aSize, uint8_t * expectedSize)
{
    MQTT_fixed_header_t test;
    uint8_t * ptr_test = (uint8_t*)&(test);
    switch (expected_return_value)
    {
        case 1:
            TEST_ASSERT_EQUAL_UINT8(expected_return_value, set_size(&test, aSize));
            TEST_ASSERT_EQUAL_UINT8(aSize, ptr_test[1]);
            break;
        case 2:
            TEST_ASSERT_EQUAL_UINT8(expected_return_value, set_size(&test, aSize));
            TEST_ASSERT_EQUAL_UINT8(expectedSize[0], ptr_test[1]);
            TEST_ASSERT_EQUAL_UINT8(expectedSize[1], ptr_test[2]);
            break;
        case 3:
            TEST_ASSERT_EQUAL_UINT8(expected_return_value, set_size(&test, aSize));
            TEST_ASSERT_EQUAL_UINT8(expectedSize[0], ptr_test[1]);
            TEST_ASSERT_EQUAL_UINT8(expectedSize[1], ptr_test[2]);
            TEST_ASSERT_EQUAL_UINT8(expectedSize[2], ptr_test[3]);
            break;
        case 4:
            TEST_ASSERT_EQUAL_UINT8(expected_return_value, set_size(&test, aSize));
            TEST_ASSERT_EQUAL_UINT8(expectedSize[0], ptr_test[1]);
            TEST_ASSERT_EQUAL_UINT8(expectedSize[1], ptr_test[2]);
            TEST_ASSERT_EQUAL_UINT8(expectedSize[2], ptr_test[3]);
            TEST_ASSERT_EQUAL_UINT8(expectedSize[3], ptr_test[4]);
            break;
        default:
            TEST_ASSERT_EQUAL_UINT8(expected_return_value, set_size(&test, aSize));
            break;
    }
}

void test_encode_fixed_header_size_tiny()
{
    /* MQTT tiny size shall return 1 as lenght */
    uint8_t expectedSize[] = {0x00};
    test_encode_fixed_header_size(1, 0, expectedSize);
    expectedSize[0] = 0x15;
    test_encode_fixed_header_size(1, 0x15, expectedSize);
    expectedSize[0] = 0x7f;
    test_encode_fixed_header_size(1, 0x7F, expectedSize);
}

void test_encode_fixed_header_size_small()
{
    /* MQTT small size shall return 2 as lenght */
    uint8_t expectedSize[] = {0x80, 0x01};
    test_encode_fixed_header_size(2, 128, expectedSize);
    expectedSize[0] = 0x81;
    expectedSize[1] = 0x40;
    test_encode_fixed_header_size(2, 0x2001, expectedSize);
    expectedSize[0] = 0xff;
    expectedSize[1] = 0x7f;
    test_encode_fixed_header_size(2, 16383, expectedSize);
}

void test_encode_fixed_header_size_big()
{
    /* MQTT big size shall return 3 as lenght */
    uint8_t expectedSize[] = {0x80, 0x80, 0x01};
    test_encode_fixed_header_size(3, 16384, expectedSize);
    expectedSize[0] = 0xA0;
    expectedSize[1] = 0xE2;
    expectedSize[2] = 0x10;
    test_encode_fixed_header_size(3, 0x43120, expectedSize);
    expectedSize[0] = 0xFF;
    expectedSize[1] = 0xFF;
    expectedSize[2] = 0x7F;
    test_encode_fixed_header_size(3, 2097151, expectedSize);
}

void test_encode_fixed_header_size_large()
{
    /* MQTT large size shall return 4 as lenght */
    uint8_t expectedSize[] = {0x80, 0x80, 0x80, 0x01};
    test_encode_fixed_header_size(4, 2097152, expectedSize);
    expectedSize[0] = 0x89;
    expectedSize[1] = 0xA4;
    expectedSize[2] = 0x8C;
    expectedSize[3] = 0x32;
    test_encode_fixed_header_size(4, 0x6431209, expectedSize);
    expectedSize[0] = 0xFF;
    expectedSize[1] = 0xFF;
    expectedSize[2] = 0xFF;
    expectedSize[3] = 0x7F;
    test_encode_fixed_header_size(4, 268435455, expectedSize);
}

void test_encode_fixed_header_size_invalid()
{
    /* MQTT invalid size shall return 0 as lenght */
    /* Zero lenght messages are allowed */
    uint8_t expectedSize[] = {0x00, 0x00, 0x00, 0x00};
    test_encode_fixed_header_size(0, 0x80000000, expectedSize);
    test_encode_fixed_header_size(0, -1, expectedSize);
    test_encode_fixed_header_size(0, (size_t)0xF0000000, expectedSize);
}

/****************************************************************************************
 * FIXED HEADER TESTS                                                                   *
 ****************************************************************************************/
void test_encode_fixed_header_all_zeros()
{
    /* Thest that fixed header with all zeros is really 0x0000 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS0, false, INVALIDCMD, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x0000, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_dub_set()
{
    /* Dup set and value expcted to be 0x0002 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, true, QoS0, false, INVALIDCMD, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x0002, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_qos1()
{
    /* QoS1 set and value expected to be 0x0004 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS1, false, INVALIDCMD, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x0004, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_qos2()
{
    /* QoS2 set and value expected to be 0x0008*/
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS2, false, INVALIDCMD, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x0008, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_invalid_qos()
{
    /* Try with too big QoS value - fail expected */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(0, encode_fixed_header(&fHdr, false, 4, false, INVALIDCMD, 0x00));
}

void test_encode_fixed_header_with_retain_set()
{
    /* Dup set and value expcted to be 0x0002 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS0, true, INVALIDCMD, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x0001, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_first_command()
{
    /* CMD 1 = CONNECT and value is expected to be 0x0010 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS0, false, CONNECT, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x0010, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_last_command()
{
    /* CMD 14 = DISCONNECT and value is expected to be 0x00E0 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS0, false, DISCONNECT, 0x00));
    TEST_ASSERT_EQUAL_HEX16(0x00E0, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_invalid_command()
{
    /* Try with reserved command 0xF - fail expected */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(0, encode_fixed_header(&fHdr, false, QoS0, false, 0xF, 0x00));
}

void test_encode_fixed_header_with_tiny_message()
{
    /* tiny message size < 0x80 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(2, encode_fixed_header(&fHdr, false, QoS0, false, INVALIDCMD, 0x7F));
    TEST_ASSERT_EQUAL_HEX16(0x7F00, TO_HEX_16(fHdr));
}

void test_encode_fixed_header_with_small_message()
{
    /* small message size < 0x8000 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(3, encode_fixed_header(&fHdr, false, QoS0, false, INVALIDCMD, 0x81));
    uint8_t expected[3] = {0x00, 0x81, 0x01};
    TEST_ASSERT_EQUAL_UINT8_ARRAY((void*)&expected, (void*)&fHdr, 3);
}

void test_encode_fixed_header_with_big_message()
{
    /* big message size < 0x800000 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(4, encode_fixed_header(&fHdr, false, QoS0, false, INVALIDCMD, 16386));
    uint8_t expected[4] = {0x00, 0x82, 0x80, 0x01};
    TEST_ASSERT_EQUAL_UINT8_ARRAY((void*)&expected, (void*)&fHdr, 4);
}

void test_encode_fixed_header_with_large_message()
{
    /* large message size < 0x80000000 */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(5, encode_fixed_header(&fHdr, false, QoS0, false, INVALIDCMD, 268435450));
    uint8_t expected[5] = {0x00, 0xFA, 0xFF, 0xFF, 0x7F};
    TEST_ASSERT_EQUAL_UINT8_ARRAY((void*)&expected, (void*)&fHdr, 5);
}

void test_encode_fixed_header_with_too_big_message()
{
    /* Test wit too big message > 0x80000000 failure expected */
    MQTT_fixed_header_t fHdr;
    TEST_ASSERT_EQUAL_INT8(0, encode_fixed_header(&fHdr, false, QoS0, false, INVALIDCMD, 0x8F12371F));
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{  
    UnityBegin("Encode fixed header");
    unsigned int tCntr = 1;
    /* Test set size */
    RUN_TEST(test_encode_fixed_header_size_tiny,            tCntr++);
    RUN_TEST(test_encode_fixed_header_size_small,           tCntr++);
    RUN_TEST(test_encode_fixed_header_size_big,             tCntr++);
    RUN_TEST(test_encode_fixed_header_size_large,           tCntr++);
    RUN_TEST(test_encode_fixed_header_size_invalid,         tCntr++);
    RUN_TEST(test_encode_fixed_header_all_zeros,            tCntr++);
    
    /* Test fixed header encode */
    RUN_TEST(test_encode_fixed_header_with_dub_set,         tCntr++);
    RUN_TEST(test_encode_fixed_header_with_qos1,            tCntr++);
    RUN_TEST(test_encode_fixed_header_with_qos2,            tCntr++);
    RUN_TEST(test_encode_fixed_header_with_invalid_qos,     tCntr++);
    RUN_TEST(test_encode_fixed_header_with_retain_set,      tCntr++);
    RUN_TEST(test_encode_fixed_header_with_first_command,   tCntr++);
    RUN_TEST(test_encode_fixed_header_with_last_command,    tCntr++);
    RUN_TEST(test_encode_fixed_header_with_invalid_command, tCntr++);
    RUN_TEST(test_encode_fixed_header_with_tiny_message,    tCntr++);
    RUN_TEST(test_encode_fixed_header_with_small_message,   tCntr++);
    RUN_TEST(test_encode_fixed_header_with_big_message,     tCntr++);
    RUN_TEST(test_encode_fixed_header_with_large_message,   tCntr++);
    RUN_TEST(test_encode_fixed_header_with_too_big_message, tCntr++);
    return (UnityEnd());
}
