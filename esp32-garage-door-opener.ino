#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char* ssid = "<ENTER WIFI NAME HERE>";
const char* password = "<ENTER WIFI PASSWORD HERE>";

WebServer server(80);

const int led = 2;
const int relay = 15;
const int reset = 26;
const int resetInterval = 3600000; // 1hr by default
const int lockDuration = 14000; //default 14 second lockout

int requestedState = 1; // open=0, closed=1
int currentState = 1; // open=0, closed=1
int lockExpire = 0; 

void setup(void) {
  // Setup GPIO pin modes
  pinMode(led, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(reset, OUTPUT);
  // Initialize GPIO positions
  digitalWrite(led, HIGH);
  digitalWrite(relay, HIGH);
  digitalWrite(reset, HIGH);
  
  Serial.begin(115200);

  // Connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", []() {
    server.send(200, "text/plain", "This is Joe's Garage Door!\nSend a GET request to an endpoint below: \n\n`/toggle`\t\tToggle door state. Digital 'press' of the garage door button.\n`/reset`\t\tReset device. Remotely press reset button.\n`/status`\t\tGet JSON status of garage door.\n`/setState?value=1`\tSet state of garage door. Valid states are the two values, `open=0` and `closed=1`\n");
  });
  
  server.on("/toggle", []() {
    digitalWrite(led, LOW);
    toggleGarageDoorState();
    server.send(200, "text/plain", "Pressed garage door button. Door state will toggle.\n");
    digitalWrite(led, HIGH);
  });

  server.on("/reset", []() {
    server.send(200, "text/plain", "Resetting device. Please wait 10 seconds.\n");
    delay(100);
    digitalWrite(reset, LOW);
  });

  server.on("/status", []() {
    digitalWrite(led, LOW);
    server.send(200, "text/plain", "{\"currentState\":" + String(currentState) + ",\"requestedState\":" + String(requestedState) +"}\n");
    digitalWrite(led, HIGH);
  });

  server.on("/setState", []() {
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i) == "value" ) {
          requestedState = server.arg(i).toInt();
          server.send(200, "text/plain", "{\"currentState\":" + String(currentState) + ",\"requestedState\":" + String(requestedState) +"}\n");
        }
      }
  });

  server.onNotFound([]() {
    digitalWrite(led, 1);
    String message = "Route Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void toggleGarageDoorState() {
    digitalWrite(led, LOW);
    digitalWrite(relay, HIGH);
    delay(100);
    digitalWrite(relay, LOW);
    delay(100);
    digitalWrite(relay, HIGH);
    digitalWrite(led, HIGH);
}

void loop(void) {
  server.handleClient();

  if (requestedState != currentState) {
    if (millis() >= lockExpire) {
      toggleGarageDoorState();
      currentState = requestedState;
      lockExpire = millis() + lockDuration;
    }
  }

  if (millis() > resetInterval) {
    digitalWrite(reset, LOW);
  }
  
  delay(2);//allow the cpu to switch to other tasks
}
