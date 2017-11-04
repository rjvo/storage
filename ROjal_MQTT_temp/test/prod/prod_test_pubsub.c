
#include "mqtt.h"
#include "unity.h"
#include <stdio.h>

#include "help.h"
#include "prod_help.h"

void prod_test_pubsub()
{
    TEST_ASSERT_TRUE_MESSAGE(enable_(4, "prod_test_d"), "Connect failed");

    char sub[] = "prod/test2";
    TEST_ASSERT_TRUE_MESSAGE(mqtt_subscribe(sub, strlen(sub), 10), "Subscribe failed");
    for (int i = 0; i < 35 ; i++) {
        if ( i % 10 == 0) {
            printf("\n");
            TEST_ASSERT_TRUE_MESSAGE(mqtt_publish("prod/test2", 10, "PROD testing", 12), "Publish failed");
            asleep(1000);
        } else {
            asleep(1000);
        }
        printf(".");
        fflush(stdout);
        TEST_ASSERT_TRUE_MESSAGE(mqtt_keepalive(1000), "keepalive failed")
    }
    printf("\n");
    TEST_ASSERT_TRUE_MESSAGE(disable_(), "Disconnect failed");
}


/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("Production pubsub tests");
    unsigned int tCntr = 1;
    RUN_TEST(prod_test_pubsub,                 tCntr++);
    return (UnityEnd());
}
