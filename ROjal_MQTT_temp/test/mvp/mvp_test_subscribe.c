
#include "mqtt.h"
#include "unity.h"
#include<stdio.h>

#include "help.h"
#include "mvp_help.h"
extern bool mvp_subscribe_test2;

void mvp_test_sub()
{
    TEST_ASSERT_TRUE_MESSAGE(mvp_enable_(0, "JAMKtestMVP3"), "Connect failed");
    TEST_ASSERT_TRUE_MESSAGE(mvp_subscribe_("mvp/test2"), "Subscribe failed");

    TEST_ASSERT_TRUE_MESSAGE(mvp_publish_("mvp/test2", "MVP testing"), "Publish failed");
    asleep(500);
    TEST_ASSERT_TRUE_MESSAGE(mvp_subscribe_test2, "Subscribe test failed - invalid content");
    TEST_ASSERT_TRUE_MESSAGE(mvp_disable_(), "Disconnect failed");
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("MVP subscribe tests");
    RUN_TEST(mvp_test_sub, 1);
    return (UnityEnd());
}
