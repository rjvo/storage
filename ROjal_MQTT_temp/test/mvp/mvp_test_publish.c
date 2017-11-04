
#include "mqtt.h"
#include "unity.h"
#include<stdio.h>

#include "help.h"
#include "mvp_help.h"

void mvp_test_publish()
{
    TEST_ASSERT_TRUE_MESSAGE(mvp_enable_(0, "JAMKtestMVP2"), "Connect failed");

    TEST_ASSERT_TRUE_MESSAGE(mvp_publish_("mvp/test1", "MVP testing"), "Publish failed");
    asleep(500);

    TEST_ASSERT_TRUE_MESSAGE(mvp_disable_(), "Disconnect failed");
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("MVP publish tests");
    RUN_TEST(mvp_test_publish, 1);
    return (UnityEnd());
}
