#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ESP32-6WD Final Pins

#define LEFT_L_EN   32
#define LEFT_R_EN   33
#define LEFT_L_PWM  18
#define LEFT_R_PWM  19

#define RIGHT_L_EN  26
#define RIGHT_R_EN  25
#define RIGHT_L_PWM 14
#define RIGHT_R_PWM 27

#define BLUE_LED 2

// Latest default tuning
int speedLimit = 255;
const int rampStep = 8;
const int rampInterval = 18;

int currentLeft = 0;
int currentRight = 0;
int targetLeft = 0;
int targetRight = 0;

unsigned long lastRampTime = 0;

// If direction is reversed, change only these
bool invertLeft = false;
bool invertRight = false;

// Nordic UART BLE UUID
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *txCharacteristic;
bool deviceConnected = false;

void sendBleMsg(String msg) {
  Serial.println(msg);
  if (deviceConnected && txCharacteristic) {
    txCharacteristic->setValue(msg.c_str());
    txCharacteristic->notify();
  }
}

void setLeftMotor(int spd) {
  spd = constrain(spd, -255, 255);

  if (invertLeft) spd = -spd;

  if (spd > 0) {
    analogWrite(LEFT_L_PWM, spd);
    analogWrite(LEFT_R_PWM, 0);
  } else if (spd < 0) {
    analogWrite(LEFT_L_PWM, 0);
    analogWrite(LEFT_R_PWM, -spd);
  } else {
    analogWrite(LEFT_L_PWM, 0);
    analogWrite(LEFT_R_PWM, 0);
  }
}

void setRightMotor(int spd) {
  spd = constrain(spd, -255, 255);

  if (invertRight) spd = -spd;

  if (spd > 0) {
    analogWrite(RIGHT_L_PWM, spd);
    analogWrite(RIGHT_R_PWM, 0);
  } else if (spd < 0) {
    analogWrite(RIGHT_L_PWM, 0);
    analogWrite(RIGHT_R_PWM, -spd);
  } else {
    analogWrite(RIGHT_L_PWM, 0);
    analogWrite(RIGHT_R_PWM, 0);
  }
}

void applyMotors() {
  setLeftMotor(currentLeft);
  setRightMotor(currentRight);

  if (currentLeft != 0 || currentRight != 0) {
    digitalWrite(BLUE_LED, HIGH);
  } else {
    digitalWrite(BLUE_LED, LOW);
  }
}

void stopCar() {
  targetLeft = 0;
  targetRight = 0;
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  sendBleMsg("Command: " + cmd);

  if (cmd == "F") {
    targetLeft = speedLimit;
    targetRight = speedLimit;
  }
  else if (cmd == "B") {
    targetLeft = -speedLimit;
    targetRight = -speedLimit;
  }
  else if (cmd == "L") {
    targetLeft = -speedLimit;
    targetRight = speedLimit;
  }
  else if (cmd == "R") {
    targetLeft = speedLimit;
    targetRight = -speedLimit;
  }
  else if (cmd == "S") {
    stopCar();
  }
  else if (cmd.startsWith("V")) {
    int v = cmd.substring(1).toInt();
    speedLimit = constrain(v, 0, 255);
    sendBleMsg("Speed set: " + String(speedLimit));
  }
}

void updateRamp() {
  if (millis() - lastRampTime < rampInterval) return;
  lastRampTime = millis();

  if (currentLeft < targetLeft) currentLeft += rampStep;
  if (currentLeft > targetLeft) currentLeft -= rampStep;

  if (currentRight < targetRight) currentRight += rampStep;
  if (currentRight > targetRight) currentRight -= rampStep;

  if (abs(currentLeft - targetLeft) < rampStep) currentLeft = targetLeft;
  if (abs(currentRight - targetRight) < rampStep) currentRight = targetRight;

  currentLeft = constrain(currentLeft, -255, 255);
  currentRight = constrain(currentRight, -255, 255);

  applyMotors();
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    deviceConnected = true;
    Serial.println("iPhone connected");
    digitalWrite(BLUE_LED, HIGH);
  }

  void onDisconnect(BLEServer* server) {
    deviceConnected = false;
    stopCar();
    currentLeft = 0;
    currentRight = 0;
    applyMotors();

    Serial.println("iPhone disconnected");
    delay(500);
    BLEDevice::startAdvertising();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();

    Serial.print("BLE Length: ");
    Serial.println(value.length());

    Serial.print("BLE Received: ");
    Serial.println(value);

    if (value.length() > 0) {
      handleCommand(value);
    } else {
      Serial.println("Empty value received");
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_L_EN, OUTPUT);
  pinMode(LEFT_R_EN, OUTPUT);
  pinMode(LEFT_L_PWM, OUTPUT);
  pinMode(LEFT_R_PWM, OUTPUT);

  pinMode(RIGHT_L_EN, OUTPUT);
  pinMode(RIGHT_R_EN, OUTPUT);
  pinMode(RIGHT_L_PWM, OUTPUT);
  pinMode(RIGHT_R_PWM, OUTPUT);

  pinMode(BLUE_LED, OUTPUT);

  digitalWrite(LEFT_L_EN, HIGH);
  digitalWrite(LEFT_R_EN, HIGH);
  digitalWrite(RIGHT_L_EN, HIGH);
  digitalWrite(RIGHT_R_EN, HIGH);

  stopCar();
  applyMotors();

  BLEDevice::init("ESP32_6WD_BLE");

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);

  txCharacteristic = service->createCharacteristic(
    TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  txCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *rxCharacteristic = service->createCharacteristic(
    RX_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  rxCharacteristic->setCallbacks(new RxCallbacks());

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  Serial.println("ESP32_6WD_BLE ready");
  Serial.println("Send: F B L R S V150 V255");
}

void loop() {
  updateRamp();
}