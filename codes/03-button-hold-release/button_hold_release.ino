#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

const char* ssid = "ESP32-6WD";
const char* password = "12345678";

WebServer server(80);

// LEFT DRIVER
#define LEFT_L_EN   32
#define LEFT_R_EN   33
#define LEFT_RPWM   19
#define LEFT_LPWM   18

// RIGHT DRIVER
#define RIGHT_L_EN  26
#define RIGHT_R_EN  25
#define RIGHT_RPWM  27
#define RIGHT_LPWM  14

#define BLUE_LED 2

int speedValue = 150;

bool invertLeft = false;
bool invertRight = false;

// Smooth movement settings
int currentLeftSpeed = 0;
int currentRightSpeed = 0;
int targetLeftSpeed = 0;
int targetRightSpeed = 0;

unsigned long lastRampTime = 0;
const int rampStep = 5;
const int rampInterval = 20;

char driveMode = 'S';   // S, F, B
char steerMode = 'N';   // N, L, R

void setupPWM() {
  ledcAttach(LEFT_RPWM, 1000, 8);
  ledcAttach(LEFT_LPWM, 1000, 8);
  ledcAttach(RIGHT_RPWM, 1000, 8);
  ledcAttach(RIGHT_LPWM, 1000, 8);
}

void enableDrivers() {
  digitalWrite(LEFT_L_EN, HIGH);
  digitalWrite(LEFT_R_EN, HIGH);
  digitalWrite(RIGHT_L_EN, HIGH);
  digitalWrite(RIGHT_R_EN, HIGH);
}

int rampValue(int current, int target) {
  if (current < target) {
    current += rampStep;
    if (current > target) current = target;
  } else if (current > target) {
    current -= rampStep;
    if (current < target) current = target;
  }
  return current;
}

void writeLeftMotor(int spd) {
  bool forward = spd >= 0;
  int pwm = abs(spd);

  if (invertLeft) forward = !forward;

  if (pwm == 0) {
    ledcWrite(LEFT_RPWM, 0);
    ledcWrite(LEFT_LPWM, 0);
    return;
  }

  if (forward) {
    ledcWrite(LEFT_LPWM, 0);
    ledcWrite(LEFT_RPWM, pwm);
  } else {
    ledcWrite(LEFT_RPWM, 0);
    ledcWrite(LEFT_LPWM, pwm);
  }
}

void writeRightMotor(int spd) {
  bool forward = spd >= 0;
  int pwm = abs(spd);

  if (invertRight) forward = !forward;

  if (pwm == 0) {
    ledcWrite(RIGHT_RPWM, 0);
    ledcWrite(RIGHT_LPWM, 0);
    return;
  }

  if (forward) {
    ledcWrite(RIGHT_LPWM, 0);
    ledcWrite(RIGHT_RPWM, pwm);
  } else {
    ledcWrite(RIGHT_RPWM, 0);
    ledcWrite(RIGHT_LPWM, pwm);
  }
}

void applyMovement() {
  enableDrivers();

  int s = speedValue;
  int slow = speedValue * 0.40;

  if (driveMode == 'F') {
    if (steerMode == 'L') {
      targetLeftSpeed = slow;
      targetRightSpeed = s;
    } else if (steerMode == 'R') {
      targetLeftSpeed = s;
      targetRightSpeed = slow;
    } else {
      targetLeftSpeed = s;
      targetRightSpeed = s;
    }
  }

  else if (driveMode == 'B') {
    if (steerMode == 'L') {
      targetLeftSpeed = -slow;
      targetRightSpeed = -s;
    } else if (steerMode == 'R') {
      targetLeftSpeed = -s;
      targetRightSpeed = -slow;
    } else {
      targetLeftSpeed = -s;
      targetRightSpeed = -s;
    }
  }

  else {
    if (steerMode == 'L') {
      targetLeftSpeed = -s;
      targetRightSpeed = s;
    } else if (steerMode == 'R') {
      targetLeftSpeed = s;
      targetRightSpeed = -s;
    } else {
      targetLeftSpeed = 0;
      targetRightSpeed = 0;
    }
  }
}

void updateMotorsSmooth() {
  if (millis() - lastRampTime < rampInterval) return;
  lastRampTime = millis();

  currentLeftSpeed = rampValue(currentLeftSpeed, targetLeftSpeed);
  currentRightSpeed = rampValue(currentRightSpeed, targetRightSpeed);

  writeLeftMotor(currentLeftSpeed);
  writeRightMotor(currentRightSpeed);

  if (currentLeftSpeed != 0 || currentRightSpeed != 0 || targetLeftSpeed != 0 || targetRightSpeed != 0) {
    digitalWrite(BLUE_LED, HIGH);
  } else {
    digitalWrite(BLUE_LED, LOW);
  }
}

String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ESP32-6WD</title>

<style>
*{
  user-select:none;
  -webkit-user-select:none;
  -webkit-touch-callout:none;
  -webkit-tap-highlight-color:transparent;
  touch-action:manipulation;
  box-sizing:border-box;
}

html, body {
  margin:0;
  padding:0;
  width:100%;
  min-height:100%;
  overflow-x:hidden;
  overflow-y:auto;
  background:#fff;
  font-family:'Poppins','Arial Black',Arial,sans-serif;
  overscroll-behavior:contain;
}

.main {
  width:100vw;
  min-height:100dvh;
  padding:32px 14px 24px;
  display:flex;
  flex-direction:column;
  align-items:center;
}

h1 {
  margin:0;
  font-size:35px;
  font-weight:900;
  color:#111;
  text-shadow:0 6px 6px rgba(0,0,0,0.5);
}

h3 {
  margin:8px 0 0;
  font-size:17px;
  font-weight:900;
  color:#222;
  letter-spacing:0.5px;
}

.controls {
  position:relative;
  width:305px;
  height:315px;
  margin-top:26px;
}

.btn {
  position:absolute;
  width:78px;
  height:78px;
  border:none;
  border-radius:22px;
  background:linear-gradient(145deg,#ffffff,#e8e8e8);
  box-shadow:8px 8px 18px rgba(0,0,0,0.5),
             -7px -7px 16px rgba(255,255,255,0.95);
  display:flex;
  justify-content:center;
  align-items:center;
}

.btn:active {
  transform:translateY(5px) scale(0.96);
  box-shadow:inset 6px 6px 13px rgba(0,0,0,0.18),
             inset -6px -6px 13px rgba(255,255,255,0.9);
}

.btn.active {
  transform:translateY(5px) scale(0.96);
  background:linear-gradient(145deg,#00d4ff,#0077ff);
  box-shadow:0 0 0 5px rgba(0,140,255,0.25),
             0 0 25px rgba(0,140,255,0.75),
             inset 5px 5px 12px rgba(0,0,0,0.25),
             inset -5px -5px 12px rgba(255,255,255,0.35);
}

.btn.active .up {
  border-bottom-color:white;
}

.btn.active .down {
  border-top-color:white;
}

.stop {
  width:100px;
  height:100px;
  border-radius:50%;
  left:102px;
  top:107px;
  background:radial-gradient(circle at 30% 25%,#ff7b7b,#e60000 60%,#720000);
  color:white;
  font-size:14px;
  font-weight:900;
  letter-spacing:1.5px;
  text-shadow:0 3px 5px rgba(0,0,0,0.75);
  box-shadow:0 14px 0 #650000,
             0 24px 28px rgba(0,0,0,0.32);
}

.stop:active {
  transform:translateY(10px) scale(0.96);
  box-shadow:0 5px 0 #650000,
             0 14px 18px rgba(0,0,0,0.25);
}

.forward { left:113px; top:6px; }
.left    { left:5px; top:118px; }
.right   { right:5px; top:118px; }
.back    { left:113px; top:230px; }

.arrow { width:0; height:0; }

.up {
  border-left:16px solid transparent;
  border-right:16px solid transparent;
  border-bottom:27px solid #111;
}

.down {
  border-left:16px solid transparent;
  border-right:16px solid transparent;
  border-top:27px solid #111;
}

.leftA {
  border-top:16px solid transparent;
  border-bottom:16px solid transparent;
  border-right:27px solid #111;
}

.rightA {
  border-top:16px solid transparent;
  border-bottom:16px solid transparent;
  border-left:27px solid #111;
}

.speedBox {
  margin-top:20px;
  width:310px;
  height:95px;
  border-radius:50px;
  background:linear-gradient(145deg,#ffffff,#eeeeee);
  box-shadow:8px 8px 10px rgba(0,0,0,0.3),
             -7px -7px 17px rgba(255,255,255,0.95);
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
}

.speedBox label {
  font-size:30px;
  font-weight:900;
  margin-bottom:25px;
  text-shadow:0 2px 3px rgba(0,0,0,0.25);
}

.speedRow {
  display:flex;
  align-items:center;
  gap:12px;
}

input[type=range] {
  width:210px;
  height:8px;
  appearance:none;
  border-radius:20px;
  background:linear-gradient(90deg,#0b8cff,#00d4ff);
}

input[type=range]::-webkit-slider-thumb {
  appearance:none;
  width:38px;
  height:38px;
  border-radius:50%;
  background:radial-gradient(circle at 30% 30%,#ffffff,#65c4ff);
  box-shadow:0 7px 16px rgba(0,0,0,0.25);
}

#speedVal {
  font-size:33px;
  font-weight:900;
  width:50px;
  text-align:left;
  letter-spacing:1px;
  color:#111;
  text-shadow:0 1px 0 #ffffff,
              0 2px 4px rgba(0,0,0,0.55),
              0 0 8px rgba(255,255,255,0.9);
}

@media (orientation: landscape) {
  html, body {
    height:auto;
    overflow-y:auto;
  }

  .main {
    min-height:100dvh;
    height:auto;
    padding:12px 22px 18px;
    display:grid;
    grid-template-columns:1fr 340px;
    grid-template-rows:auto auto 1fr;
    column-gap:22px;
    row-gap:4px;
    align-items:center;
    justify-items:center;
  }

  h1 {
    grid-column:1 / -1;
    font-size:28px;
    line-height:1;
  }

  h3 {
    grid-column:1 / -1;
    font-size:14px;
    margin:0;
  }

  .controls {
    grid-column:1;
    grid-row:3;
    width:285px;
    height:245px;
    margin-top:0;
    transform:scale(0.82);
    transform-origin:center;
  }

  .speedBox {
    grid-column:2;
    grid-row:3;
    margin-top:0;
    width:315px;
    height:130px;
  }

  .speedBox label {
    font-size:26px;
    margin-bottom:10px;
  }

  input[type=range] {
    width:210px;
  }

  #speedVal {
    font-size:30px;
  }
}

@media (orientation: landscape) and (max-height:390px) {
  .main {
    align-items:start;
    padding-top:8px;
    row-gap:0;
  }

  h1 {
    font-size:24px;
  }

  h3 {
    font-size:12px;
  }

  .controls {
    transform:scale(0.74);
  }

  .speedBox {
    height:105px;
    transform:scale(0.92);
  }
}
</style>
</head>

<body>
<div class="main">
  <h1>ESP32-6WD</h1>
  <h3>Hold Button Release Controls</h3>

  <div class="controls">
    <button id="forwardBtn" class="btn forward" onclick="toggleDrive('F')">
      <div class="arrow up"></div>
    </button>

    <button class="btn left" onpointerdown="steer('L')" onpointerup="steer('N')" onpointercancel="steer('N')" onpointerleave="steer('N')">
      <div class="arrow leftA"></div>
    </button>

    <button class="btn stop" onclick="stopCar()">STOP</button>

    <button class="btn right" onpointerdown="steer('R')" onpointerup="steer('N')" onpointercancel="steer('N')" onpointerleave="steer('N')">
      <div class="arrow rightA"></div>
    </button>

    <button id="backBtn" class="btn back" onclick="toggleDrive('B')">
      <div class="arrow down"></div>
    </button>
  </div>

  <div class="speedBox">
    <label>Speed</label>
    <div class="speedRow">
      <input id="speedSlider" type="range" min="0" max="255" value="150" oninput="setSpeed(this.value)">
      <div id="speedVal">59%</div>
    </div>
  </div>
</div>

<script>
document.addEventListener('contextmenu', e => e.preventDefault());
document.addEventListener('selectstart', e => e.preventDefault());
document.addEventListener('copy', e => e.preventDefault());
document.addEventListener('cut', e => e.preventDefault());
document.addEventListener('dragstart', e => e.preventDefault());

let driveMode = 'S';

function send(c) {
  fetch('/cmd?move=' + c);
}

function updateButtons() {
  document.getElementById('forwardBtn').classList.toggle('active', driveMode === 'F');
  document.getElementById('backBtn').classList.toggle('active', driveMode === 'B');
}

function toggleDrive(mode) {
  if (driveMode === mode) {
    driveMode = 'S';
    send('S');
  } else {
    driveMode = mode;
    send(mode);
  }
  updateButtons();
}

function steer(mode) {
  send(mode);
}

function stopCar() {
  driveMode = 'S';
  send('S');
  updateButtons();
}

function getSpeedColor(percent) {
  let hue = 210 - (percent * 2.1);
  return "hsl(" + hue + ", 100%, 45%)";
}

function setSpeed(v) {
  let percent = Math.round((v / 255) * 100);
  let color = getSpeedColor(percent);

  document.getElementById('speedVal').innerHTML = percent + "%";

  document.getElementById('speedSlider').style.background =
    "linear-gradient(90deg, " + color + " 0%, " + color + " " + percent + "%, #e5e5e5 " + percent + "%, #e5e5e5 100%)";

  fetch('/speed?value=' + v);
}

setSpeed(150);
updateButtons();
</script>

</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", page);
}

void handleCmd() {
  if (!server.hasArg("move")) {
    server.send(400, "text/plain", "NO CMD");
    return;
  }

  String m = server.arg("move");

  if (m == "F") {
    driveMode = 'F';
    steerMode = 'N';
  }
  else if (m == "B") {
    driveMode = 'B';
    steerMode = 'N';
  }
  else if (m == "L") {
    steerMode = 'L';
  }
  else if (m == "R") {
    steerMode = 'R';
  }
  else if (m == "N") {
    steerMode = 'N';
  }
  else {
    driveMode = 'S';
    steerMode = 'N';
  }

  applyMovement();
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    speedValue = server.arg("value").toInt();
    speedValue = constrain(speedValue, 0, 255);
    applyMovement();
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_L_EN, OUTPUT);
  pinMode(LEFT_R_EN, OUTPUT);
  pinMode(RIGHT_L_EN, OUTPUT);
  pinMode(RIGHT_R_EN, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  setupPWM();
  enableDrivers();
  applyMovement();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  esp_wifi_set_max_tx_power(78);

  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/speed", handleSpeed);

  server.begin();
}

void loop() {
  server.handleClient();
  updateMotorsSmooth();
}