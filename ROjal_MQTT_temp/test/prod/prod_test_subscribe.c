
#include "mqtt.h"
#include "unity.h"
#include <string.h>

#include "help.h"
#include "prod_help.h"

void prod_test_subscribe()
{
    TEST_ASSERT_TRUE_MESSAGE(enable_(0, "prod_test_c"), "Connect failed");

    char sub[] = "prod/test2";
    TEST_ASSERT_TRUE_MESSAGE(mqtt_subscribe(sub, strlen(sub), 10), "Subscribe failed");

    TEST_ASSERT_TRUE_MESSAGE(mqtt_publish("prod/test2", 10, "PROD testing", 12), "Publish failed");
    asleep(500);
    TEST_ASSERT_TRUE_MESSAGE(disable_(), "Disconnect failed");
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("Production subscribe tests");
    unsigned int tCntr = 1;
    RUN_TEST(prod_test_subscribe,              tCntr++);
    return (UnityEnd());
}
