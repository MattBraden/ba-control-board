#include "Config.h"

using namespace config;
using namespace led;

Preferences prefs;

void ConfigHelper::saveConfig(std::map<int, LEDStateMachine>& fullConfig) {
  String configString = convertConfigToJson(fullConfig);
  prefs.begin("config");
  prefs.putString("config", configString.c_str());
  prefs.end();
}

void ConfigHelper::configChange(std::map<int, LEDStateMachine>& fullConfig, LEDConfig& config) {
  std::map<int, LEDStateMachine>::iterator it = fullConfig.find(config.input);
  if (it == fullConfig.end()) {
    fullConfig.insert({config.input, config.stateMachine});
  } else {
    it->second = config.stateMachine;
  }

  saveConfig(fullConfig);
}

void ConfigHelper::loadFullConfig(std::map<int, LEDStateMachine>& fullConfig) {
    prefs.begin("config");
    char buffer[5000];
    prefs.getString("config", buffer, 5000);
    prefs.end();

    DynamicJsonBuffer jsonBuffer(2000);
    JsonObject& root = jsonBuffer.parseObject(String(buffer));
    for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
      if (root.containsKey(String(i))) {
        root[String(i)]["i"] = i;  // TRASH: This is pretty hacky... but in our config the input is just stored as a key in the map
        LEDConfig conf = convertJsonToConfig(root[String(i)]);
        configChange(fullConfig, conf);
      }
    }
}

// TODO: This should prolly call config change instead of doing a change itself..
bool ConfigHelper::deleteConfig(std::map<int, LEDStateMachine>& fullConfig, int input) {
  std::map<int, LEDStateMachine>::iterator it = fullConfig.find(input);
  if (it == fullConfig.end()) {
    return false;
  } else {
    it->second.disable();
    fullConfig.erase(input);
    return true;
  }
  saveConfig(fullConfig);
}

String ConfigHelper::convertConfigToJson(std::map<int, LEDStateMachine>& fullConfig) {
  DynamicJsonBuffer jsonBuffer(5000);
  JsonObject& root = jsonBuffer.createObject();

  for (auto const& x: fullConfig ) {
    int input = x.first;
    LEDStateMachine stateMachine = x.second;

    JsonObject& config = root.createNestedObject(String(input));
    config["p"] = stateMachine.getPriority();
    JsonArray& outputStates = config.createNestedArray("s");
    for (LEDState& ledState : stateMachine.getStates()) {
      JsonObject& outputState = outputStates.createNestedObject();
      JsonArray& outputStateOutput = outputState.createNestedArray("o");
      for (int out : ledState.getOutput()) {
        outputStateOutput.add(out);
      }
      outputState["b"] = ledState.getBrightness();
      outputState["t"] = ledState.getTime();
    }
  }

  String output;
  root.printTo(output);
  return output;
}

LEDConfig ConfigHelper::convertJsonToConfig(JsonObject& root) {
  int input = root["i"];

  vector<LEDState> ledStates = {};
  for (JsonObject& jsonState : root["s"].as<JsonArray>()) {
    vector<int> outputs = {};
    for (JsonVariant& output : jsonState["o"].as<JsonArray>()) {
      outputs.push_back(output.as<int>());
    }
    LEDState state(outputs, jsonState["t"].as<int>(), jsonState["b"].as<int>());
    ledStates.push_back(state);
  }
  LEDStateMachine stateMachine(ledStates, root["p"]);
  return LEDConfig {input, stateMachine};
}