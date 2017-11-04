
#include "mqtt.h"
#include "unity.h"
#include <stdio.h>

#include "help.h"
#include "prod_help.h"

void prod_test_publish()
{
    TEST_ASSERT_TRUE_MESSAGE(enable_(0, "prod_test_b"), "Connect failed");

    TEST_ASSERT_TRUE_MESSAGE(mqtt_publish("prod/test1", 10, "PROD testing", 12), "Publish failed");
    asleep(500);

    TEST_ASSERT_TRUE_MESSAGE(disable_(), "Disconnect failed");
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("Production tests publish");
    unsigned int tCntr = 1;
    RUN_TEST(prod_test_publish,                tCntr++);
    return (UnityEnd());
}
