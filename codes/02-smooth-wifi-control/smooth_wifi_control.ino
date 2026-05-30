#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_ROVER";
const char* password = "12345678";

WebServer server(80);

#define ENA 25
#define IN1 26
#define IN2 27

#define ENB 14
#define IN3 12
#define IN4 13

int currentSpeed = 0;
int targetSpeed = 0;
int maxSpeed = 220;

String currentDirection = "stop";

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/forward", []() { setDirection("forward"); });
  server.on("/backward", []() { setDirection("backward"); });
  server.on("/left", []() { setDirection("left"); });
  server.on("/right", []() { setDirection("right"); });
  server.on("/stop", []() { setDirection("stop"); });

  server.begin();
}

void loop() {
  server.handleClient();
  smoothSpeedUpdate();
}

void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>Smooth ESP32 Rover</title>
    <style>
      body { text-align:center; font-family:Arial; background:#111; color:white; }
      button { width:150px; height:60px; font-size:20px; margin:10px; border-radius:12px; }
    </style>
  </head>
  <body>
    <h1>Smooth Wi-Fi Rover Control</h1>
    <button onclick="fetch('/forward')">Forward</button><br>
    <button onclick="fetch('/left')">Left</button>
    <button onclick="fetch('/stop')">Stop</button>
    <button onclick="fetch('/right')">Right</button><br>
    <button onclick="fetch('/backward')">Backward</button>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void setDirection(String dir) {
  currentDirection = dir;

  if (dir == "stop") {
    targetSpeed = 0;
  } else {
    targetSpeed = maxSpeed;
    applyDirection(dir);
  }

  server.send(200, "text/plain", dir);
}

void smoothSpeedUpdate() {
  if (currentSpeed < targetSpeed) {
    currentSpeed += 5;
    if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
  } else if (currentSpeed > targetSpeed) {
    currentSpeed -= 5;
    if (currentSpeed < targetSpeed) currentSpeed = targetSpeed;
  }

  analogWrite(ENA, currentSpeed);
  analogWrite(ENB, currentSpeed);

  if (currentSpeed == 0 && currentDirection == "stop") {
    stopMotors();
  }

  delay(20);
}

void applyDirection(String dir) {
  if (dir == "forward") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }

  else if (dir == "backward") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }

  else if (dir == "left") {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }

  else if (dir == "right") {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}