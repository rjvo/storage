
#include "mqtt.h"
#include "unity.h"
#include <stdio.h>

#include "help.h"
#include "prod_help.h"

void prod_test_a()
{
    TEST_ASSERT_TRUE_MESSAGE(enable_(0, "prod_test_a"), "Connect failed");
    TEST_ASSERT_TRUE_MESSAGE(disable_(), "Disconnect failed");
}

/****************************************************************************************
 * TEST main                                                                            *
 ****************************************************************************************/
int main(void)
{
    UnityBegin("Production connect tests");
    unsigned int tCntr = 1;
    RUN_TEST(prod_test_a,                    tCntr++);
    return (UnityEnd());
}
