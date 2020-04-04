#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "esp_log.h"
#include "Preferences.h"

#include <map>
#include <mutex>
#include <stdlib.h>

#include "LED.h"
#include "JSON.h"

using namespace led;
using namespace std;

const char* ssid     = "BAEng";
const char* password = "123456789";

Preferences prefs;
AsyncWebServer server(80);

struct Config {
  int input;
	LEDStateMachine stateMachine;
};

std::map<int, LEDStateMachine> configMap;
std::mutex g_configMap_mutex;

String convertConfigToJson();
Config convertJsonToConfig(JsonObject& root);

void setupLEDs() {
  for (uint8_t x: pwmChannelLookup ) {
    ledcSetup(x, PWM_FREQ, PWM_RES);
  }

  uint8_t i = 0;
  for (uint8_t x: ledPinLookup) {
    ledcAttachPin(x, pwmChannelLookup[i]);
    ledcWrite(pwmChannelLookup[i], 0);
    ++i;
  }
}

void setupInputs() {
  for (uint8_t x: inputPinLookup ) {
    pinMode(x, INPUT_PULLDOWN);
  }
}

void saveConfig() {
  String configString = convertConfigToJson();
  prefs.begin("config");
  prefs.putString("config", configString.c_str());
  prefs.end();
}

void configChange(Config& config) {
  std::map<int, LEDStateMachine>::iterator it = configMap.find(config.input);
  if (it == configMap.end()) {
    configMap.insert({config.input, config.stateMachine});
  } else {
    it->second = config.stateMachine;
  }

  saveConfig();
}

void loadFullConfig(String json) {
    DynamicJsonBuffer jsonBuffer(2000);
    JsonObject& root = jsonBuffer.parseObject(json);
    for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
      if (root.containsKey(String(i))) {
        root[String(i)]["i"] = i;  // This is pretty hacky... but in our config the input is just stored as a key in the map
        Config conf = convertJsonToConfig(root[String(i)]);
        configChange(conf);
      }
    }
}

void getSavedConfig() {
  prefs.begin("config");
  char buffer[5000];
  prefs.getString("config", buffer, 5000);
  prefs.end();

  loadFullConfig(String(buffer));
}

bool deleteConfig(int input) {
  std::map<int, LEDStateMachine>::iterator it = configMap.find(input);
  if (it == configMap.end()) {
    return false;
  } else {
    it->second.disable();
    configMap.erase(input);
    return true;
  }
  saveConfig();
}

String convertConfigToJson() {
  DynamicJsonBuffer jsonBuffer(5000);
  JsonObject& root = jsonBuffer.createObject();

  for (auto const& x: configMap ) {
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

// This is for a single config
Config convertJsonToConfig(JsonObject& root) {
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
    return Config {input, stateMachine};
}

void setupWifi() {
  IPAddress local_ip(69, 69, 69, 69);
  IPAddress gateway(69, 69, 69, 69);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  //WiFi.begin("2Guys1Flat", "caliboy20");
}

void setupHttpServer() {
  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
    g_configMap_mutex.lock();
    JsonObject& jsonObj = json.as<JsonObject>();
    Config config = convertJsonToConfig(jsonObj);
    configChange(config);
    request->send(200, "application/json", "{\"success\": true}");
    g_configMap_mutex.unlock();
  });
  server.addHandler(handler);
 
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", convertConfigToJson());
  });

  server.on("^\\/config\\/([0-9]+)$", HTTP_DELETE, [] (AsyncWebServerRequest *request) {
    g_configMap_mutex.lock();
    String input = request->pathArg(0);
    bool success = deleteConfig(input.toInt());
    if (success) {
      request->send(200, "application/json", "{\"success\": true}");
    } else {
      request->send(400, "application/json", "{\"success\": false}");
    }
    g_configMap_mutex.unlock();
  });
 
  server.begin();
}

void setup(){
  //esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // enable WARN logs from WiFi stack
  Serial.begin(115200);

  getSavedConfig();
  setupWifi();
  setupInputs();
  setupLEDs();
  setupHttpServer();  
}

// TODO: Config should not also keep statemachine states....
// Who coded this...
std::map<int, LEDOutput> determineOutput(std::map<int, bool> inputs) {
  g_configMap_mutex.lock();

  // Set all outputs to off
  std::map<int, LEDOutput> outputs;
  for(int i = 0; i < NUMBER_OF_OUTPUTS; ++i) {
    outputs.insert({i, LEDOutput{0, 0, 0}});
  }

  // Figure out what statemachines are active
  std::vector<LEDStateMachine*> activeStateMachines;
  for (auto &x: configMap) {
    int input = x.first;
    LEDStateMachine &stateMachine = configMap.find(input)->second;

    if (inputs[input]) {
      stateMachine.check();
      activeStateMachines.push_back(&stateMachine);
    } else {
      stateMachine.disable();
    }
  }

  
  // Of the active stateMachine, set outputs based on priority
  // This code is slowly turning into a gigantic mess.

  for (auto stateMachine: activeStateMachines) {
    LEDState state = stateMachine->getCurrentState();
    for (int output : state.getOutput()) {
      if (stateMachine->getPriority() > outputs[output].priority) {
        if (outputs[output].stateMachine != 0) {
          outputs[output].stateMachine->disable();
        }
        outputs[output] = LEDOutput{state.getBrightness(), stateMachine->getPriority(), stateMachine};
      }
    }
  }

  g_configMap_mutex.unlock();
  return outputs;
}

std::map<int, bool> getInputs() {
  std::map<int, bool> inputs;

  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
    if (digitalRead(inputPinLookup[i])) {
      inputs.insert({i, true});
    } else {
      inputs.insert({i, false});
    }
  }

  return inputs;
}


// This is kind of confusing, since we use the PWM to determine if a LED is on or not.
std::map<int, LEDOutput> g_current_output_state;
void doOutput(std::map<int, LEDOutput> outputs) {
  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
    if (g_current_output_state[i].brightness != outputs[i].brightness) {
      ledcWrite(pwmChannelLookup[i], outputs[i].brightness);
    }
  }
  g_current_output_state = outputs;
}

void loop() {
  std::map<int, bool> inputs = getInputs();
  std::map<int, LEDOutput> outputs = determineOutput(inputs);
  // char data[100];
  // for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
  //   sprintf(data, "Input %d = %d", i, inputs[i]);
  //   Serial.println(data);
  //   sprintf(data, "Output %d = %d", i, outputs[i].brightness);
  //   Serial.println(data);
  // }
  doOutput(outputs);
}