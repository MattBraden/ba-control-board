#pragma once
#include "Preferences.h"
#include "LED.h"
#include "ArduinoJson.h"

#include <map>

using namespace led;

namespace config {
    // Global config is the following
    //std::map<int, LEDStateMachine> g_full_config;

    // LEDConfig refers to a single input -> output
    struct LEDConfig {
        int input;
        led::LEDStateMachine stateMachine;
    };

    class ConfigHelper {
        public: 
            static int convertBrightness(int oldValue);
            static void saveConfig(std::map<int, LEDStateMachine>& fullConfig);
            static void configChange(std::map<int, LEDStateMachine>& fullConfig, LEDConfig& config);
            static void loadFullConfig(std::map<int, LEDStateMachine>& fullConfig);
            static void getSavedConfig(std::map<int, LEDStateMachine>& fullConfig);
            static bool deleteConfig(std::map<int, LEDStateMachine>& fullConfig, int input);
            static String convertConfigToJson(std::map<int, led::LEDStateMachine>& fullConfig);
            static LEDConfig convertJsonToConfig(JsonObject& root);
    };
}