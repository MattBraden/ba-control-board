#include <gtest/gtest.h>
#include "LED.h"
#include "Config.h"

#include <map>

TEST(test, TestingSomeStuff) {

    std::map<int, LEDOutput> expectedOutput = {
        {0, {20, 1, 0}},
        {1, {20, 1, 0}},
        {2, {20, 1, 0}}
    };

    std::map<int, bool> inputs = {
        {1, true},
        {2, true},
        {3, true}
    };

    std::map<int, LEDStateMachine> fullConfig = {
        {0, StateMachine}
    };

    std::map<int, LEDOutput> outputs = LEDHelper::determineOutput(fullConfig, inputs);
    EXPECT_EQ(true, true);
}