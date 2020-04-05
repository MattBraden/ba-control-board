#include "Arduino.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "esp_log.h"

#include <map>
#include <mutex>
#include <stdlib.h>

#include "LED.h"
#include "Config.h"

using namespace led;
using namespace config;
using namespace std;

const char* ssid     = "BAEng";
const char* password = "123456789";

AsyncWebServer server(80);

std::map<int, LEDStateMachine> g_fullConfig;
std::mutex g_fullConfig_mutex;

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
    g_fullConfig_mutex.lock();
    JsonObject& jsonObj = json.as<JsonObject>();
    LEDConfig config = ConfigHelper::convertJsonToConfig(jsonObj);
    ConfigHelper::configChange(g_fullConfig, config);
    request->send(200, "application/json", "{\"success\": true}");
    g_fullConfig_mutex.unlock();
  });
  server.addHandler(handler);
 
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", ConfigHelper::convertConfigToJson(g_fullConfig));
  });

  server.on("^\\/config\\/([0-9]+)$", HTTP_DELETE, [] (AsyncWebServerRequest *request) {
    g_fullConfig_mutex.lock();
    String input = request->pathArg(0);
    bool success = ConfigHelper::deleteConfig(g_fullConfig, input.toInt());
    if (success) {
      request->send(200, "application/json", "{\"success\": true}");
    } else {
      request->send(400, "application/json", "{\"success\": false}");
    }
    g_fullConfig_mutex.unlock();
  });
 
  server.begin();
}

void setup(){
  //esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // enable WARN logs from WiFi stack
  Serial.begin(115200);

  ConfigHelper::loadFullConfig(g_fullConfig);
  setupWifi();
  setupInputs();
  setupLEDs();
  setupHttpServer();  
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
// We could make this value static
std::map<int, LEDOutput> g_currentOutputStates;
void doOutput(std::map<int, LEDOutput> outputs) {
  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
    if (g_currentOutputStates[i].brightness != outputs[i].brightness) {
      ledcWrite(pwmChannelLookup[i], outputs[i].brightness);
    }
  }
  g_currentOutputStates = outputs;
}

void loop() {
  std::map<int, bool> inputs = getInputs();
  std::map<int, LEDOutput> outputs = LEDHelper::determineOutput(g_fullConfig, inputs);
  /*
  char data[100];
  for(int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
    sprintf(data, "Input %d = %d", i, inputs[i]);
    Serial.println(data);
    sprintf(data, "Output %d = %d", i, outputs[i].brightness);
    Serial.println(data);
  }
  */
  doOutput(outputs);
}