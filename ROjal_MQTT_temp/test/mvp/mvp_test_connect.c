
#include "mqtt.h"
#include "unity.h"
#include<stdio.h>

#include "help.h"
#include "mvp_help.h"

void mvp_test_connect()
{
    TEST_ASSERT_TRUE_MESSAGE(mvp_enable_(0, "JAMKtestMVP1"), "Connect failed");
    TEST_ASSERT_TRUE_MESSAGE(mvp_disable_(), "Disconnect failed");
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("MVP connect tests");
    RUN_TEST(mvp_test_connect, 1);
    return (UnityEnd());
}
