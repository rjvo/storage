
#include "mqtt.h"
#include "unity.h"
#include<stdio.h>

#include "help.h"
#include "mvp_help.h"
extern bool mvp_subscribe_test2;

void mvp_test_pubsub()
{
    TEST_ASSERT_TRUE_MESSAGE(mvp_enable_(4, "JAMKtestMVP4"), "Connect failed");

    TEST_ASSERT_TRUE_MESSAGE(mvp_subscribe_("mvp/test2"), "Subscribe failed");
    for (int i = 0; i < (60 /* * 60 * 20 */) ; i++) {
        if ( i % 10 == 0) {
            mvp_subscribe_test2 = false;
            TEST_ASSERT_TRUE_MESSAGE(mvp_publish_("mvp/test2", "MVP testing"), "Publish failed");
            asleep(1000);
            TEST_ASSERT_TRUE_MESSAGE(mvp_subscribe_test2, "Subscribe test failed - invalid content");
        } else {
            asleep(1000);
        }
        TEST_ASSERT_TRUE_MESSAGE(mvp_keepalive_(1000), "keepalive failed")
    }

    TEST_ASSERT_TRUE_MESSAGE(mvp_disable_(), "Disconnect failed");
}


/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("MVP pubsub tests");
    RUN_TEST(mvp_test_pubsub, 1);
    return (UnityEnd());
}
