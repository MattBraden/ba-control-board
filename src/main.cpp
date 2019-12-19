#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "esp_log.h"
#include <Preferences.h>
#include "EEPROM.h"

#include <map>
#include <mutex>
#include <stdlib.h>

#include "LED.h"


using namespace led;
using namespace std;

const char* ssid     = "MattehB";
const char* password = "caliboy19";

Preferences prefs;
AsyncWebServer server(80);

struct Config {
  int input;
	LEDStateMachine stateMachine;
};

std::map<int, LEDStateMachine> configMap;
std::mutex g_configMap_mutex;

void setupLEDs() {
  ledcSetup(LED_1_PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcSetup(LED_2_PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcSetup(LED_3_PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_1_PIN, LED_1_PWM_CHAN);
  ledcAttachPin(LED_2_PIN, LED_2_PWM_CHAN);
  ledcAttachPin(LED_3_PIN, LED_3_PWM_CHAN);
}

void setupInputs() {
  pinMode(INPUT_1_PIN, INPUT_PULLDOWN);
  pinMode(INPUT_2_PIN, INPUT_PULLDOWN);
  pinMode(INPUT_3_PIN, INPUT_PULLDOWN);
}

void configChange(Config& config) {
  std::map<int, LEDStateMachine>::iterator it = configMap.find(config.input);
  if (it == configMap.end()) {
    configMap.insert({config.input, config.stateMachine});
  } else {
    it->second.disable();
    it->second = config.stateMachine;
  }

  /*
    char* in;
    itoa(config.input, in, 10);
    prefs.begin(in);

    prefs.putBytes(in, *configMap, 24 * sizeof(LEDStateMachine::LEDStateMachine));    
  */
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
}

String convertConfigToJson() {
  DynamicJsonBuffer jsonBuffer(5000);
  JsonObject& root = jsonBuffer.createObject();

  for (auto const& x: configMap ) {
    int input = x.first;
    LEDStateMachine stateMachine = x.second;

    JsonArray& outputStates = root.createNestedArray(String(input));
    for (LEDState& ledState : stateMachine.getStates()) {
      JsonObject& outputState = outputStates.createNestedObject();
      JsonArray& outputStateOutput = outputState.createNestedArray("output");
      for (int out : ledState.getOutput()) {
        outputStateOutput.add(out);
      }
      outputState["brighness"] = ledState.getBrightness();
      outputState["time"] = ledState.getTime();
    }
  }

  String output;
  root.printTo(output);
  return output;
}

Config convertJsonToConfig(JsonObject& root) {
    // Input Parsing
    int input = root["input"];

    vector<LEDState> ledStates = {};
    for (JsonObject& jsonState : root["outputStates"].as<JsonArray>()) {
      vector<int> outputs = {};
      for (JsonVariant& output : jsonState["output"].as<JsonArray>()) {
        outputs.push_back(output.as<int>());
      }
      LEDState state(outputs, jsonState["time"].as<int>(), jsonState["brightness"].as<int>());
      ledStates.push_back(state);
    }
    LEDStateMachine stateMachine(ledStates);
    return Config {input, stateMachine};
}

void setup(){
  //EEPROM.get(0, configMap);
  //esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // enable WARN logs from WiFi stack
  Serial.begin(115200);

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println(WiFi.localIP());


  // TODO: Better status codes, its always a success

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
  setupInputs();
  setupLEDs();
}

void checkInputs() {
  g_configMap_mutex.lock();
  for (auto const& x: configMap ) {
    int input = x.first;
    LEDStateMachine& stateMachine = configMap.find(input)->second;

    if (digitalRead(inputPinLookup[input])) {
      stateMachine.check();
    } else {
      if (!stateMachine.isDisabled()) {
        stateMachine.disable();
      }
    }
  }
  g_configMap_mutex.unlock();
}

void loop(){
  checkInputs();
}

// How to handle priority of shared outputs