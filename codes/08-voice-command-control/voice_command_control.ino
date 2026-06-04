#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

const char* ssid = "ESP32-6WD";
const char* password = "12345678";

#define L_EN1 32
#define L_EN2 33
#define L_PWM1 18
#define L_PWM2 19

#define R_EN1 26
#define R_EN2 25
#define R_PWM1 14
#define R_PWM2 27

#define BLUE_LED 2
#define HEADLIGHT_PIN 23
#define HORN_PIN 13

int speedLimit = 255;
int rampStep = 8;
int rampInterval = 18;

int currentSpeedPercent = 100;
int currentPWM = 255;

int turnPulseTime = 280;

int leftTarget = 0;
int rightTarget = 0;
int leftCurrent = 0;
int rightCurrent = 0;

unsigned long lastRampTime = 0;
unsigned long autoStopTime = 0;
bool autoStopActive = false;

bool pulseHorn = false;
bool pulseLight = false;
bool hornOneShot = false;

unsigned long hornOffTime = 0;
unsigned long lastPulseTime = 0;
bool pulseState = false;

const int pulseInterval = 166;

void setup() {
  Serial.begin(115200);

  pinMode(BLUE_LED, OUTPUT);
  pinMode(HEADLIGHT_PIN, OUTPUT);
  pinMode(HORN_PIN, OUTPUT);

  pinMode(L_EN1, OUTPUT);
  pinMode(L_EN2, OUTPUT);
  pinMode(L_PWM1, OUTPUT);
  pinMode(L_PWM2, OUTPUT);

  pinMode(R_EN1, OUTPUT);
  pinMode(R_EN2, OUTPUT);
  pinMode(R_PWM1, OUTPUT);
  pinMode(R_PWM2, OUTPUT);

  digitalWrite(L_EN1, HIGH);
  digitalWrite(L_EN2, HIGH);
  digitalWrite(R_EN1, HIGH);
  digitalWrite(R_EN2, HIGH);

  stopAll();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/cmd", handleCommand);
  server.begin();
}

void loop() {
  server.handleClient();
  smoothRamp();
  handleAutoStop();
  handleHornAndLight();
}

void handleRoot() {
  String page = "";
  page += "<h2>ESP32 6WD Siri Control</h2>";
  page += "<p>Current Speed: " + String(currentSpeedPercent) + "%</p>";
  server.send(200, "text/html", page);
}

void handleCommand() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "Missing command");
    return;
  }

  String c = server.arg("c");
  c.toUpperCase();

  if (c == "F") moveForward();
  else if (c == "B") moveBackward();
  else if (c == "S" || c == "STOP") stopAll();

  else if (c == "RL" || c == "L") rotateLeft();
  else if (c == "RR" || c == "R") rotateRight();

  else if (c == "TL") turnLeftPulse();
  else if (c == "TR") turnRightPulse();

  else if (c == "F1") moveForwardFor(1000);
  else if (c == "F2") moveForwardFor(2000);
  else if (c == "F3") moveForwardFor(3000);

  else if (c == "B1") moveBackwardFor(1000);
  else if (c == "B2") moveBackwardFor(2000);
  else if (c == "B3") moveBackwardFor(3000);

  else if (c == "V25") setSpeedPercent(25);
  else if (c == "V50") setSpeedPercent(50);
  else if (c == "V75") setSpeedPercent(75);
  else if (c == "V100") setSpeedPercent(100);

  else if (c == "HORN") hornOneSecond();
  else if (c == "PHON") startPulseHorn();
  else if (c == "PHOFF") stopPulseHorn();

  else if (c == "LON") headlightOn();
  else if (c == "LOFF") headlightOff();
  else if (c == "PLON") startPulseLight();
  else if (c == "PLOFF") stopPulseLight();

  server.send(200, "text/plain", "OK: " + c);
}

void setSpeedPercent(int percent) {
  currentSpeedPercent = constrain(percent, 0, 100);
  currentPWM = map(currentSpeedPercent, 0, 100, 0, speedLimit);

  if (leftTarget > 0) leftTarget = currentPWM;
  else if (leftTarget < 0) leftTarget = -currentPWM;

  if (rightTarget > 0) rightTarget = currentPWM;
  else if (rightTarget < 0) rightTarget = -currentPWM;

  digitalWrite(BLUE_LED, HIGH);
  delay(80);
  digitalWrite(BLUE_LED, LOW);
}

void moveForward() {
  autoStopActive = false;
  leftTarget = -currentPWM;
  rightTarget = -currentPWM;
  digitalWrite(BLUE_LED, HIGH);
}

void moveBackward() {
  autoStopActive = false;
  leftTarget = currentPWM;
  rightTarget = currentPWM;
  digitalWrite(BLUE_LED, HIGH);
}

void rotateLeft() {
  autoStopActive = false;
  leftTarget = currentPWM;
  rightTarget = -currentPWM;
  digitalWrite(BLUE_LED, HIGH);
}

void rotateRight() {
  autoStopActive = false;
  leftTarget = -currentPWM;
  rightTarget = currentPWM;
  digitalWrite(BLUE_LED, HIGH);
}

void turnLeftPulse() {
  leftTarget = currentPWM;
  rightTarget = -currentPWM;
  autoStopTime = millis() + turnPulseTime;
  autoStopActive = true;
  digitalWrite(BLUE_LED, HIGH);
}

void turnRightPulse() {
  leftTarget = -currentPWM;
  rightTarget = currentPWM;
  autoStopTime = millis() + turnPulseTime;
  autoStopActive = true;
  digitalWrite(BLUE_LED, HIGH);
}

void moveForwardFor(int durationMs) {
  leftTarget = -currentPWM;
  rightTarget = -currentPWM;
  autoStopTime = millis() + durationMs;
  autoStopActive = true;
  digitalWrite(BLUE_LED, HIGH);
}

void moveBackwardFor(int durationMs) {
  leftTarget = currentPWM;
  rightTarget = currentPWM;
  autoStopTime = millis() + durationMs;
  autoStopActive = true;
  digitalWrite(BLUE_LED, HIGH);
}

void handleAutoStop() {
  if (autoStopActive && millis() >= autoStopTime) {
    stopMotorsOnly();
    autoStopActive = false;
  }
}

void stopMotorsOnly() {
  leftTarget = 0;
  rightTarget = 0;
  digitalWrite(BLUE_LED, LOW);
}

void stopAll() {
  leftTarget = 0;
  rightTarget = 0;

  pulseHorn = false;
  pulseLight = false;
  hornOneShot = false;
  autoStopActive = false;

  digitalWrite(HORN_PIN, LOW);
  digitalWrite(HEADLIGHT_PIN, LOW);
  digitalWrite(BLUE_LED, LOW);
}

void hornOneSecond() {
  pulseHorn = false;
  hornOneShot = true;
  hornOffTime = millis() + 1000;
  digitalWrite(HORN_PIN, HIGH);
}

void startPulseHorn() {
  hornOneShot = false;
  pulseHorn = true;
}

void stopPulseHorn() {
  pulseHorn = false;
  digitalWrite(HORN_PIN, LOW);
}

void headlightOn() {
  pulseLight = false;
  digitalWrite(HEADLIGHT_PIN, HIGH);
}

void headlightOff() {
  pulseLight = false;
  digitalWrite(HEADLIGHT_PIN, LOW);
}

void startPulseLight() {
  pulseLight = true;
}

void stopPulseLight() {
  pulseLight = false;
  digitalWrite(HEADLIGHT_PIN, LOW);
}

void handleHornAndLight() {
  if (hornOneShot && millis() >= hornOffTime) {
    hornOneShot = false;
    digitalWrite(HORN_PIN, LOW);
  }

  if (millis() - lastPulseTime >= pulseInterval) {
    lastPulseTime = millis();
    pulseState = !pulseState;

    if (pulseHorn) {
      digitalWrite(HORN_PIN, pulseState ? HIGH : LOW);
    }

    if (pulseLight) {
      digitalWrite(HEADLIGHT_PIN, pulseState ? HIGH : LOW);
    }
  }
}

void smoothRamp() {
  if (millis() - lastRampTime < rampInterval) return;
  lastRampTime = millis();

  if (leftCurrent < leftTarget) {
    leftCurrent += rampStep;
    if (leftCurrent > leftTarget) leftCurrent = leftTarget;
  } else if (leftCurrent > leftTarget) {
    leftCurrent -= rampStep;
    if (leftCurrent < leftTarget) leftCurrent = leftTarget;
  }

  if (rightCurrent < rightTarget) {
    rightCurrent += rampStep;
    if (rightCurrent > rightTarget) rightCurrent = rightTarget;
  } else if (rightCurrent > rightTarget) {
    rightCurrent -= rampStep;
    if (rightCurrent < rightTarget) rightCurrent = rightTarget;
  }

  driveMotor(leftCurrent, rightCurrent);
}

void driveMotor(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  if (leftSpeed > 0) {
    analogWrite(L_PWM1, leftSpeed);
    analogWrite(L_PWM2, 0);
  } else if (leftSpeed < 0) {
    analogWrite(L_PWM1, 0);
    analogWrite(L_PWM2, abs(leftSpeed));
  } else {
    analogWrite(L_PWM1, 0);
    analogWrite(L_PWM2, 0);
  }

  if (rightSpeed > 0) {
    analogWrite(R_PWM1, rightSpeed);
    analogWrite(R_PWM2, 0);
  } else if (rightSpeed < 0) {
    analogWrite(R_PWM1, 0);
    analogWrite(R_PWM2, abs(rightSpeed));
  } else {
    analogWrite(R_PWM1, 0);
    analogWrite(R_PWM2, 0);
  }
}