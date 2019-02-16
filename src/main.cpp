#include "ESPAsyncWebServer.h"
#include "Wifi.h"
#include "ArduinoJson.h"
#include "esp_log.h"
#include "SPI.h"

// Replace with your network credentials
//const char* ssid     = "BAEngineering";
//const char* password = "123456789";
const char* ssid     = "TheJabronis";
const char* password = "Breezytree6";

// Assign output variables to GPIO pins
//const int output23 = 23;
const int output22 = 22;
const int output21 = 21;

AsyncWebServer server(80);

struct Input {
  bool i1;
  bool i2;
};

struct ConfigRequest {
  
};

// LEARN: when declaring a struct like this, you need to do new struct() to declare memory location otherwise it will fail when you try to 
// access the variable.
struct Input *input_p = new Input();

enum SEQUENTIAL_ENUM {STATE0, STATE1, STATE2, STATE3};
long interval = 200;
uint8_t state = STATE0;

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

void handleActionRequest(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  
  // Based off request do something...
  if (input_p->i1 == true) {
    input_p->i1 = false;
  } else {
    input_p->i1 = true;
  }

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["i1"] = input_p->i1;
  root["ssid"] = WiFi.SSID();
  root.printTo(*response);
  request->send(response);
}

void configChange(const char* input, const char* output) {
  
}

bool handleBody(AsyncWebServerRequest *request, uint8_t *datas) {

  Serial.printf("[REQUEST]\t%s\r\n", (const char*)datas);
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& jsonObject = jsonBuffer.parseObject((const char*)datas); 
  if (!jsonObject.success()) return 0;

  if (jsonObject.containsKey("input")) {
    Serial.println(jsonObject["input"].asString());
    configChange(jsonObject["input"].as<char*>(), jsonObject["output"].as<char*>());
    return 1;
  };  
  return 0;
}

void setup(){
  esp_log_level_set("wifi", ESP_LOG_VERBOSE);      // enable WARN logs from WiFi stack
  Serial.begin(115200);

  // Initialize the output variables as outputs
  //pinMode(output23, OUTPUT);
  pinMode(output22, OUTPUT);
  pinMode(output21, OUTPUT);
  // Set outputs to LOW
  //digitalWrite(output23, LOW);
  digitalWrite(output22, LOW);
  digitalWrite(output21, LOW);

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
 
  server.on("/action", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Worked!");
  });

  server.on("/action", HTTP_POST, handleActionRequest);
 
  server.begin();
}

void checkInputs(struct Input *input) {
  /*
  if (IOPIN1) {
    input0->i1 = true;
  } else {
    input->i1 = false;
  }
  if (IO_PIN2) {
    input->i2 = true;
  } else {
    input->i2 = false;
  }
  */
}

void doOutputs() {
  if (input_p->i1 == true) {
    sequential();
  } else {
    //digitalWrite(output23, LOW);
    digitalWrite(output22, LOW);
    digitalWrite(output21, LOW);
  }
}

void loop(){

  // This is the guts of the program.  It is going to be difficult to make all of this changeable via an app
  // This should use interrupts
  //checkInputs();

  // Use inputs to turn on signals
  doOutputs();
}