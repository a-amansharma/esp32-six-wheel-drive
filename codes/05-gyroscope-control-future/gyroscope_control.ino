#include <WiFi.h>
#include <WebServer.h>

// ===== WIFI AP =====
const char* ssid = "ESP32-6WD-GYRO";
const char* password = "12345678";

IPAddress phoneIP(192,168,4,2);
const int phonePort = 80;

WebServer server(80);

// ===== SAME PINOUT =====
#define L_LEN   32
#define L_REN   33
#define L_LPWM  18
#define L_RPWM  19

#define R_LEN   26
#define R_REN   25
#define R_LPWM  14
#define R_RPWM  27

#define LED_PIN 2

// ===== SETTINGS =====
int speedLimit = 140;
const int rampStep = 5;
const int rampInterval = 20;

int currentLeft = 0, currentRight = 0;
int targetLeft = 0, targetRight = 0;

float zeroX = 0, zeroY = 0;
bool calibrated = false;

unsigned long lastRampTime = 0;
unsigned long lastReadTime = 0;
unsigned long lastDataTime = 0;

bool tiltActive = false;

// ===== MOTOR =====
void leftMotor(int v) {
  v = constrain(v, -255, 255);
  if (v > 0) {
    ledcWrite(L_RPWM, v);
    ledcWrite(L_LPWM, 0);
  } else if (v < 0) {
    ledcWrite(L_RPWM, 0);
    ledcWrite(L_LPWM, -v);
  } else {
    ledcWrite(L_RPWM, 0);
    ledcWrite(L_LPWM, 0);
  }
}

void rightMotor(int v) {
  v = constrain(v, -255, 255);
  if (v > 0) {
    ledcWrite(R_RPWM, v);
    ledcWrite(R_LPWM, 0);
  } else if (v < 0) {
    ledcWrite(R_RPWM, 0);
    ledcWrite(R_LPWM, -v);
  } else {
    ledcWrite(R_RPWM, 0);
    ledcWrite(R_LPWM, 0);
  }
}

void stopCar() {
  targetLeft = targetRight = 0;
  currentLeft = currentRight = 0;
  leftMotor(0);
  rightMotor(0);
  tiltActive = false;
  digitalWrite(LED_PIN, LOW);
}

void setTargets(int l, int r) {
  targetLeft = constrain(l, -speedLimit, speedLimit);
  targetRight = constrain(r, -speedLimit, speedLimit);
}

void updateMotors() {
  if (millis() - lastRampTime < rampInterval) return;
  lastRampTime = millis();

  if (currentLeft < targetLeft) currentLeft += rampStep;
  if (currentLeft > targetLeft) currentLeft -= rampStep;
  if (currentRight < targetRight) currentRight += rampStep;
  if (currentRight > targetRight) currentRight -= rampStep;

  if (abs(currentLeft - targetLeft) < rampStep) currentLeft = targetLeft;
  if (abs(currentRight - targetRight) < rampStep) currentRight = targetRight;

  leftMotor(currentLeft);
  rightMotor(currentRight);
}

// ===== SIMPLE JSON VALUE PARSER =====
bool getValue(String data, const char* key, float &value) {
  String k = "\"" + String(key) + "\"";
  int p = data.indexOf(k);
  if (p < 0) return false;

  int b = data.indexOf("[", p);
  int e = data.indexOf("]", b);
  if (b < 0 || e < 0) return false;

  String arr = data.substring(b + 1, e);
  int comma = arr.lastIndexOf(",");
  String val = comma >= 0 ? arr.substring(comma + 1) : arr;
  val.trim();

  if (val == "null" || val.length() == 0) return false;
  value = val.toFloat();
  return true;
}

bool readPhyphox(float &x, float &y, float &z) {
  WiFiClient client;

  if (!client.connect(phoneIP, phonePort)) {
    return false;
  }

  client.print("GET /get?accX&accY&accZ&x&y&z HTTP/1.1\r\n");
  client.print("Host: 192.168.4.2\r\n");
  client.print("Connection: close\r\n\r\n");

  String data = "";
  unsigned long timeout = millis();

  while (client.connected() && millis() - timeout < 1200) {
    while (client.available()) {
      data += char(client.read());
    }
  }

  client.stop();

  bool okX = getValue(data, "accX", x);
  bool okY = getValue(data, "accY", y);
  bool okZ = getValue(data, "accZ", z);

  if (!okX) okX = getValue(data, "x", x);
  if (!okY) okY = getValue(data, "y", y);
  if (!okZ) okZ = getValue(data, "z", z);

  return okX && okY;
}

int mapTilt(float v) {
  float deadZone = 1.2;
  float maxTilt = 6.0;

  if (abs(v) < deadZone) return 0;

  float sign = v > 0 ? 1 : -1;
  float mag = abs(v);
  if (mag > maxTilt) mag = maxTilt;

  mag = (mag - deadZone) / (maxTilt - deadZone) * 100.0;
  return (int)(sign * mag);
}

void processPhoneData() {
  if (millis() - lastReadTime < 80) return;
  lastReadTime = millis();

  float x, y, z;
  bool ok = readPhyphox(x, y, z);

  if (!ok) {
    if (millis() - lastDataTime > 700) {
      stopCar();
    }
    return;
  }

  lastDataTime = millis();

  if (!calibrated) {
    zeroX = x;
    zeroY = y;
    calibrated = true;

    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(80);
      digitalWrite(LED_PIN, LOW);
      delay(80);
    }
  }

  float tx = x - zeroX;
  float ty = y - zeroY;

  int turn = -mapTilt(tx);
  int drive = mapTilt(ty);

  tiltActive = (abs(turn) > 8 || abs(drive) > 8);

  digitalWrite(LED_PIN, tiltActive ? HIGH : LOW);

  int throttle = map(drive, -100, 100, -speedLimit, speedLimit);
  int steering = map(turn, -100, 100, speedLimit, -speedLimit);

  int leftOut = -(throttle + steering);
  int rightOut = -(throttle - steering);

  setTargets(leftOut, rightOut);
}

// ===== WEB PAGE FOR STOP / SPEED =====
void handleRoot() {
  server.send(200, "text/html",
  "<html><body style='font-family:Arial;text-align:center'>"
  "<h2>ESP32 6WD phyphox Gyro</h2>"
  "<p>Keep phyphox open on iPhone: Raw Sensors → Acceleration with g</p>"
  "<button style='font-size:25px;background:red;color:white;padding:20px;border-radius:15px' onclick=\"fetch('/S')\">STOP</button>"
  "<br><br>Speed <input type='range' min='0' max='255' value='140' oninput=\"fetch('/speed?value='+this.value)\">"
  "</body></html>");
}

void handleStop() {
  stopCar();
  server.send(200, "text/plain", "STOP");
}

void handleSpeed() {
  if (server.hasArg("value")) speedLimit = constrain(server.arg("value").toInt(), 0, 255);
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  pinMode(L_LEN, OUTPUT);
  pinMode(L_REN, OUTPUT);
  pinMode(R_LEN, OUTPUT);
  pinMode(R_REN, OUTPUT);

  digitalWrite(L_LEN, HIGH);
  digitalWrite(L_REN, HIGH);
  digitalWrite(R_LEN, HIGH);
  digitalWrite(R_REN, HIGH);

  ledcAttach(L_RPWM, 1000, 8);
  ledcAttach(L_LPWM, 1000, 8);
  ledcAttach(R_RPWM, 1000, 8);
  ledcAttach(R_LPWM, 1000, 8);

  stopCar();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);
  server.on("/S", handleStop);
  server.on("/speed", handleSpeed);
  server.begin();

  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(120);
    digitalWrite(LED_PIN, LOW);
    delay(120);
  }
}

void loop() {
  server.handleClient();
  processPhoneData();
  updateMotors();
}