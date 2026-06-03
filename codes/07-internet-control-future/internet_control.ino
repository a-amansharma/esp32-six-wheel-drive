#include <WiFi.h>
#include <PubSubClient.h>

#define BLUE_LED 2
#define BUZZER_PIN 13
#define HEADLIGHT_PIN 23

// CHANGE THESE ONLY
const char* ssid = "iPhone";
const char* password = "244466666";

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* controlTopic = "esp32_6wd_aman_v7/control";

WiFiClient espClient;
PubSubClient client(espClient);

// Existing wiring - same as your 6WD rover
#define L_EN1 32
#define L_EN2 33
#define L_PWM1 18
#define L_PWM2 19

#define R_EN1 26
#define R_EN2 25
#define R_PWM1 14
#define R_PWM2 27

int speedLimit = 140;
int rampStep = 8;
int rampInterval = 18;
int trimValue = 0;   // -25 to +25

int currentLeft = 0;
int currentRight = 0;
int targetLeft = 0;
int targetRight = 0;

unsigned long lastRamp = 0;
unsigned long lastCommandTime = 0;
const unsigned long internetSafetyTimeout = 1800;
const unsigned long hornSafetyTimeout = 900;

bool continuousHorn = false;
unsigned long lastHornCommand = 0;
unsigned long minHornUntil = 0;
bool hornStopAfterMinimum = false;
const unsigned long minHornBeepMs = 500;

bool hornPulseMode = false;
bool lightBlinkMode = false;
bool normalHeadlightOn = false;
bool pulseState = false;
unsigned long lastPulseToggle = 0;
const unsigned long pulseHalfInterval = 165;  // smoother blink/beep: about 3 times per second

bool buzzerOutputOn = false;
unsigned long bluePulseUntil = 0;

unsigned long ignoreMotionUntil = 0;
const unsigned long startupCommandIgnoreMs = 1500;

void hardStopRover();
void fullStopAll();

void setupPWM() {
  ledcAttach(L_PWM1, 1000, 8);
  ledcAttach(L_PWM2, 1000, 8);
  ledcAttach(R_PWM1, 1000, 8);
  ledcAttach(R_PWM2, 1000, 8);
}


void setupBuzzer() {
  ledcAttach(BUZZER_PIN, 2400, 8);
  ledcWrite(BUZZER_PIN, 0);
}

void hornOn() {
  buzzerOutputOn = true;
  ledcWrite(BUZZER_PIN, 128);
}

void hornOff() {
  buzzerOutputOn = false;
  ledcWrite(BUZZER_PIN, 0);
}

void connectedBeep() {
  // Beep only, no motor movement.
  hardStopRover();
  ledcWrite(BUZZER_PIN, 128);
  delay(1000);
  ledcWrite(BUZZER_PIN, 0);
  buzzerOutputOn = false;
  hardStopRover();
}

void updateHornAndLight() {
  unsigned long now = millis();

  if (hornPulseMode || lightBlinkMode) {
    if (now - lastPulseToggle >= pulseHalfInterval) {
      lastPulseToggle = now;
      pulseState = !pulseState;
    }
  } else {
    pulseState = false;
  }

  if (hornPulseMode) {
    hornStopAfterMinimum = false;
    if (pulseState) hornOn();
    else hornOff();
  } else {
    if (hornStopAfterMinimum && now >= minHornUntil) {
      hornStopAfterMinimum = false;
      hornOff();
    }

    if (continuousHorn && now - lastHornCommand > hornSafetyTimeout) {
      continuousHorn = false;
      hornStopAfterMinimum = false;
      hornOff();
    }
  }

  if (lightBlinkMode) {
    digitalWrite(HEADLIGHT_PIN, pulseState ? HIGH : LOW);
  } else {
    digitalWrite(HEADLIGHT_PIN, normalHeadlightOn ? HIGH : LOW);
  }
}


void pulseBlueLed(unsigned long durationMs = 220) {
  bluePulseUntil = millis() + durationMs;
}

bool isRoverMoving() {
  return targetLeft != 0 || targetRight != 0 || currentLeft != 0 || currentRight != 0;
}

void updateBlueLed() {
  bool ledState = buzzerOutputOn || isRoverMoving() || (millis() < bluePulseUntil);
  digitalWrite(BLUE_LED, ledState ? HIGH : LOW);
}

void setMotorRaw(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  if (leftSpeed > 0) {
    ledcWrite(L_PWM1, leftSpeed);
    ledcWrite(L_PWM2, 0);
  } else if (leftSpeed < 0) {
    ledcWrite(L_PWM1, 0);
    ledcWrite(L_PWM2, -leftSpeed);
  } else {
    ledcWrite(L_PWM1, 0);
    ledcWrite(L_PWM2, 0);
  }

  if (rightSpeed > 0) {
    ledcWrite(R_PWM1, rightSpeed);
    ledcWrite(R_PWM2, 0);
  } else if (rightSpeed < 0) {
    ledcWrite(R_PWM1, 0);
    ledcWrite(R_PWM2, -rightSpeed);
  } else {
    ledcWrite(R_PWM1, 0);
    ledcWrite(R_PWM2, 0);
  }
}

void setTargetDirect(int leftSpeed, int rightSpeed) {
  int l = constrain(leftSpeed, -255, 255);
  int r = constrain(rightSpeed, -255, 255);

  // Remove tiny noise/jitter commands.
  const int motorNoiseFloor = 12;
  if (abs(l) < motorNoiseFloor) l = 0;
  if (abs(r) < motorNoiseFloor) r = 0;

  if (l != 0 || r != 0) {
    l = constrain(l - trimValue, -255, 255);
    r = constrain(r + trimValue, -255, 255);
  }

  targetLeft = l;
  targetRight = r;
  lastCommandTime = millis();
  if (l != 0 || r != 0) pulseBlueLed(260);
}

void setCommand(String cmd) {
  cmd.trim();

  if (cmd == "B") {
    setTargetDirect(speedLimit, speedLimit);
  } else if (cmd == "F") {
    setTargetDirect(-speedLimit, -speedLimit);
  } else if (cmd == "L") {
    setTargetDirect(speedLimit, -speedLimit);
  } else if (cmd == "R") {
    setTargetDirect(-speedLimit, speedLimit);
  } else if (cmd == "BL") {
    setTargetDirect(speedLimit / 2, speedLimit);
  } else if (cmd == "BR") {
    setTargetDirect(speedLimit, speedLimit / 2);
  } else if (cmd == "FL") {
    setTargetDirect(-speedLimit / 2, -speedLimit);
  } else if (cmd == "FR") {
    setTargetDirect(-speedLimit, -speedLimit / 2);
  } else if (cmd == "S" || cmd == "N") {
    fullStopAll();
  }
}

void setJoystick(String msg) {
  // Format: J,x,y   x and y range: -100 to +100
  int firstComma = msg.indexOf(',');
  int secondComma = msg.indexOf(',', firstComma + 1);

  if (firstComma < 0 || secondComma < 0) return;

  int x = msg.substring(firstComma + 1, secondComma).toInt();
  int y = msg.substring(secondComma + 1).toInt();

  x = constrain(x, -100, 100);
  y = constrain(y, -100, 100);

  const int deadZone = 14;
  if (abs(x) < deadZone && abs(y) < deadZone) {
    setTargetDirect(0, 0);
    return;
  }

  int forward = map(y, -100, 100, speedLimit, -speedLimit);
  int turn = map(x, -100, 100, speedLimit, -speedLimit);

  int left = 0;
  int right = 0;

  if (abs(y) >= deadZone) {
    // Button-style corner turning:
    // Forward corners use normal turn mix.
    // Backward corners need reverse mix so BL/BR match the 8 button logic.
    if (forward > 0) {
      left = forward - (turn / 2);
      right = forward + (turn / 2);
    } else {
      left = forward + (turn / 2);
      right = forward - (turn / 2);
    }
  } else {
    // Pure left/right spin stays exactly like button mode.
    left = turn;
    right = -turn;
  }

  left = constrain(left, -speedLimit, speedLimit);
  right = constrain(right, -speedLimit, speedLimit);

  setTargetDirect(left, right);
}

void handleMQTT(String msg) {
  msg.trim();

  Serial.print("MQTT: ");
  Serial.println(msg);

  bool isMotionCommand = msg == "F" || msg == "B" || msg == "L" || msg == "R" || msg == "FL" || msg == "FR" || msg == "BL" || msg == "BR" || msg.startsWith("J,");

  if (isMotionCommand && millis() < ignoreMotionUntil) {
    hardStopRover();
    Serial.println("Startup/reconnect motion command ignored");
    return;
  }

  if (msg.startsWith("SPD,")) {
    speedLimit = constrain(msg.substring(4).toInt(), 0, 255);
    Serial.print("Speed limit: ");
    Serial.println(speedLimit);
    lastCommandTime = millis();
    return;
  }

  if (msg.startsWith("TRIM,")) {
    trimValue = constrain(msg.substring(5).toInt(), -25, 25);
    Serial.print("Trim: ");
    Serial.println(trimValue);
    lastCommandTime = millis();
    return;
  }


  if (msg == "H1") {
    if (!hornPulseMode) {
      unsigned long now = millis();
      continuousHorn = true;
      hornStopAfterMinimum = false;
      lastHornCommand = now;
      minHornUntil = now + minHornBeepMs;
      hornOn();
      pulseBlueLed(hornSafetyTimeout);
    }
    return;
  }

  if (msg == "H0") {
    continuousHorn = false;
    if (!hornPulseMode) {
      if (millis() < minHornUntil) {
        hornStopAfterMinimum = true;
      } else {
        hornStopAfterMinimum = false;
        hornOff();
      }
    }
    return;
  }

  if (msg == "HPULSE1") {
    if (!hornPulseMode) {
      hornPulseMode = true;
      continuousHorn = false;
      hornStopAfterMinimum = false;
      lastPulseToggle = millis();
      pulseState = true;
      hornOn();
      pulseBlueLed(300);
    }
    return;
  }

  if (msg == "HPULSE0") {
    hornPulseMode = false;
    continuousHorn = false;
    hornStopAfterMinimum = false;
    hornOff();
    return;
  }

  if (msg == "LIGHT1") {
    normalHeadlightOn = true;
    if (!lightBlinkMode) digitalWrite(HEADLIGHT_PIN, HIGH);
    return;
  }

  if (msg == "LIGHT0") {
    normalHeadlightOn = false;
    if (!lightBlinkMode) digitalWrite(HEADLIGHT_PIN, LOW);
    return;
  }

  if (msg == "LBLINK1") {
    if (!lightBlinkMode) {
      lightBlinkMode = true;
      lastPulseToggle = millis();
      pulseState = true;
      digitalWrite(HEADLIGHT_PIN, HIGH);
    }
    return;
  }

  if (msg == "LBLINK0") {
    lightBlinkMode = false;
    digitalWrite(HEADLIGHT_PIN, normalHeadlightOn ? HIGH : LOW);
    return;
  }

  if (msg.startsWith("J,")) {
    setJoystick(msg);
    return;
  }

  setCommand(msg);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  handleMQTT(msg);
}

void hardStopRover() {
  targetLeft = 0;
  targetRight = 0;
  currentLeft = 0;
  currentRight = 0;
  setMotorRaw(0, 0);
  lastCommandTime = millis();
}

void fullStopAll() {
  hardStopRover();
  continuousHorn = false;
  hornStopAfterMinimum = false;
  hornPulseMode = false;
  lightBlinkMode = false;
  normalHeadlightOn = false;
  pulseState = false;
  hornOff();
  digitalWrite(HEADLIGHT_PIN, LOW);
  digitalWrite(BLUE_LED, LOW);
}

void connectWiFi() {
  Serial.print("Connecting WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
    delay(300);
    Serial.print(".");
  }

  hardStopRover();
  ignoreMotionUntil = millis() + startupCommandIgnoreMs;
  digitalWrite(BLUE_LED, LOW);
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());
  connectedBeep();
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");

    String clientId = "ESP32_6WD_V7_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(controlTopic);
      hardStopRover();
      ignoreMotionUntil = millis() + startupCommandIgnoreMs;
      hornPulseMode = false;
      hornStopAfterMinimum = false;
      lightBlinkMode = false;
      normalHeadlightOn = false;
      hornOff();
      digitalWrite(HEADLIGHT_PIN, LOW);
      digitalWrite(BLUE_LED, LOW);
    } else {
      Serial.print("failed rc=");
      Serial.println(client.state());
      digitalWrite(BLUE_LED, LOW);
      delay(2000);
    }
  }
}

void rampMotor() {
  if (millis() - lastRamp >= rampInterval) {
    lastRamp = millis();

    if (currentLeft < targetLeft) currentLeft += rampStep;
    if (currentLeft > targetLeft) currentLeft -= rampStep;

    if (currentRight < targetRight) currentRight += rampStep;
    if (currentRight > targetRight) currentRight -= rampStep;

    if (abs(currentLeft - targetLeft) < rampStep) currentLeft = targetLeft;
    if (abs(currentRight - targetRight) < rampStep) currentRight = targetRight;

    setMotorRaw(currentLeft, currentRight);
  }
}


void setup() {
    pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(27, OUTPUT);

  digitalWrite(32, LOW);
  digitalWrite(33, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  digitalWrite(26, LOW);
  digitalWrite(25, LOW);
  digitalWrite(14, LOW);
  digitalWrite(27, LOW);
  Serial.begin(115200);

  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(HEADLIGHT_PIN, OUTPUT);

  pinMode(L_EN1, OUTPUT);
  pinMode(L_EN2, OUTPUT);
  pinMode(R_EN1, OUTPUT);
  pinMode(R_EN2, OUTPUT);

  digitalWrite(L_EN1, HIGH);
  digitalWrite(L_EN2, HIGH);
  digitalWrite(R_EN1, HIGH);
  digitalWrite(R_EN2, HIGH);

  setupPWM();
  setupBuzzer();
  setMotorRaw(0, 0);
  hornOff();
  digitalWrite(BLUE_LED, LOW);
  normalHeadlightOn = false;
  lightBlinkMode = false;
  digitalWrite(HEADLIGHT_PIN, LOW);

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // Safety: if internet/controller stops sending heartbeat, rover stops automatically.
  if (millis() - lastCommandTime > internetSafetyTimeout) {
    targetLeft = 0;
    targetRight = 0;
  }

  updateHornAndLight();
  rampMotor();
  updateBlueLed();
}
