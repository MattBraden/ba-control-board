#include "ESPAsyncWebServer.h"
#include "Wifi.h"
#include "ArduinoJson.h"
#include "esp_log.h"
#include <list>
#include <map>

// Replace with your network credentials
//const char* ssid     = "BAEngineering";
//const char* password = "123456789";
const char* ssid     = "TheJabronis";
const char* password = "Breezytree6";

AsyncWebServer server(80);

enum SEQUENTIAL_ENUM {STATE0, STATE1, STATE2, STATE3};
long interval = 200;
uint8_t state = STATE0;

/*
void sequential() {
  // This means it is declared once otherwise it would be declared every time we run this function
  static long currentMillis = millis();
  if (millis() - currentMillis >= interval) {
    switch(state) {
      case STATE0:
        //digitalWrite(output23, HIGH);
        state = STATE1;
        break;
      
      case STATE1:
        digitalWrite(output22, HIGH);
        state = STATE2;
        break;
        
      case STATE2:
        digitalWrite(output21, HIGH);
        state = STATE3;
        break;
        
      case STATE3:
        //digitalWrite(output23, LOW);
        digitalWrite(output22, LOW);
        digitalWrite(output21, LOW);
        state = STATE0;
        break;
    }
    currentMillis = millis();
  }
}
*/

// This will need to get a lot more complicated if we want to add sequential stuff...
// Also if we want to do AND OR logic on inputs
struct ConfigRequest {
	uint8_t input;
	uint8_t output;
	uint32_t brightness; // 0 (OFF) - 65,536 (FULL ON)
};

struct InputConfig {
  uint8_t input;
};

struct OutputConfig {
  uint8_t output;
  uint8_t brightness;
};

std::map<uint8_t, OutputConfig> stateOfWorld;

uint8_t INPUT_1_PIN = 34;
uint8_t INPUT_2_PIN = 35;
uint8_t INPUT_3_PIN = 32;

uint8_t LED_1_PIN = 23;
uint8_t LED_2_PIN = 22;
uint8_t LED_3_PIN = 18;

uint8_t LED_1_PWM_CHAN = 0;
uint8_t LED_2_PWM_CHAN = 1;
uint8_t LED_3_PWM_CHAN = 2;

const double PWM_FREQ = 5000;
const uint8_t PWM_RES = 16;

std::map<uint8_t, uint8_t> inputPinLookup = {
	{1, INPUT_1_PIN},
	{2, INPUT_2_PIN},
	{3, INPUT_3_PIN}
};

std::map<uint8_t, uint8_t> ledPinLookup = {
	{1, LED_1_PIN},
	{2, LED_2_PIN},
	{3, LED_3_PIN}
};

std::map<uint8_t, uint8_t> pwmChannelLookup = {
	{1, LED_1_PWM_CHAN},
	{2, LED_2_PWM_CHAN}, 
	{3, LED_3_PWM_CHAN}
};

void setupLEDs() {
  ledcSetup(LED_1_PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcSetup(LED_2_PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcSetup(LED_3_PWM_CHAN, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_1_PIN, LED_1_PWM_CHAN);
  ledcAttachPin(LED_2_PIN, LED_2_PWM_CHAN);
  ledcAttachPin(LED_3_PIN, LED_3_PWM_CHAN);
}

void setupInputs() {
  pinMode(INPUT_1_PIN, INPUT);
  pinMode(INPUT_2_PIN, INPUT);
  pinMode(INPUT_3_PIN, INPUT);
}

void updateStateOfWorld(uint8_t input, OutputConfig outputConfig) {
	stateOfWorld.insert(std::make_pair(input, outputConfig ));
}

void configChange(int input, int output, int brightness) {
	OutputConfig outputConfig;
	outputConfig.output = output;
	outputConfig.brightness = brightness;

	updateStateOfWorld(input, outputConfig);
}

bool handleBody(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.printf("[REQUEST]\t%s\r\n", (const char*)datas);
  
  DynamicJsonBuffer jsonBuffer(1000);
  JsonObject& jsonObject = jsonBuffer.parseObject((const char*)datas); 
  if (!jsonObject.success()) return 0;

  if (jsonObject.containsKey("input")) {
    configChange(jsonObject["input"].as<int>(), jsonObject["output"].as<int>(), jsonObject["brightness"].as<int>());
    return 1;
  };
  return 0;
}

void setup(){
  esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // enable WARN logs from WiFi stack
  Serial.begin(115200);

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println(WiFi.localIP());

  /*
  FUUUCCCCKKKK
  WiFi.softAP(ssid, password);
 
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  */

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/config") {
      if (!handleBody(request, data)) request->send(200, "text/plain", "false");
      request->send(200, "text/plain", "true");
    }
  });
 
  // Get StateOfWorld (Config)
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Worked!");
  });
 
  server.begin();

  setupInputs();
  setupLEDs();
}

void checkInputs() {
  // Loop through StateOfWorld and output if anything matches
	for (std::map<uint8_t, OutputConfig>::iterator it = stateOfWorld.begin(); it != stateOfWorld.end(); ++it) {
		uint8_t input = it->first;
		OutputConfig outputConfig = it->second;
		if (digitalRead(inputPinLookup[input])) {
			ledcWrite(pwmChannelLookup[outputConfig.output], outputConfig.brightness);
		}
		else {
			ledcWrite(pwmChannelLookup[outputConfig.output], 0);
		}
	}
}

void loop(){
  checkInputs();
}