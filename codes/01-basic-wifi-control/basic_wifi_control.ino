#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32_ROVER";
const char* password = "12345678";

WebServer server(80);

// Change these pins according to your wiring
#define ENA 25
#define IN1 26
#define IN2 27

#define ENB 14
#define IN3 12
#define IN4 13

int speedValue = 200;

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopRover();

  WiFi.softAP(ssid, password);
  Serial.println("WiFi Started");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/forward", forward);
  server.on("/backward", backward);
  server.on("/left", left);
  server.on("/right", right);
  server.on("/stop", stopRoverWeb);

  server.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 Rover</title>
    <style>
      body { text-align:center; font-family:Arial; background:#111; color:white; }
      button { width:140px; height:60px; font-size:20px; margin:10px; border-radius:12px; }
    </style>
  </head>
  <body>
    <h1>My First ESP32 Rover</h1>
    <button onclick="location.href='/forward'">Forward</button><br>
    <button onclick="location.href='/left'">Left</button>
    <button onclick="location.href='/stop'">Stop</button>
    <button onclick="location.href='/right'">Right</button><br>
    <button onclick="location.href='/backward'">Backward</button>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void forward() {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  server.sendHeader("Location", "/");
  server.send(303);
}

void backward() {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  server.sendHeader("Location", "/");
  server.send(303);
}

void left() {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  server.sendHeader("Location", "/");
  server.send(303);
}

void right() {
  analogWrite(ENA, speedValue);
  analogWrite(ENB, speedValue);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  server.sendHeader("Location", "/");
  server.send(303);
}

void stopRover() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void stopRoverWeb() {
  stopRover();
  server.sendHeader("Location", "/");
  server.send(303);
}