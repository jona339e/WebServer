#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <littleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>



const int ledPin = 2; // built-in led

bool ledState = 0;

char ssid[32];
char password[32];

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


void readConfigFile();

void notifyClients() {
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}



void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  if (!LittleFS.begin()) {
  Serial.println("An error has occurred while mounting LittleFS");
  return;
  }

  readConfigFile();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });


  // Start server
  server.begin();


}

void loop() {
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
}


void readConfigFile() {



  File file = LittleFS.open("/config.txt", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  int lineNumber = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim(); // Remove any leading or trailing whitespace
    if (lineNumber == 0) {
      // Copy SSID into the char array
      strncpy(ssid, line.c_str(), sizeof(ssid) - 1);
      ssid[sizeof(ssid) - 1] = '\0'; // Ensure null-termination
    } else if (lineNumber == 1) {
      // Copy password into the char array
      strncpy(password, line.c_str(), sizeof(password) - 1);
      password[sizeof(password) - 1] = '\0'; // Ensure null-termination
    }
    lineNumber++;
  }


  file.close();

  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);



} 

