#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

const char* ssid = "ESP32-6WD";
const char* password = "12345678";

WebServer server(80);

#define LEFT_L_EN   32
#define LEFT_R_EN   33
#define LEFT_RPWM   19
#define LEFT_LPWM   18

#define RIGHT_L_EN  26
#define RIGHT_R_EN  25
#define RIGHT_RPWM  27
#define RIGHT_LPWM  14

#define BLUE_LED 2

int speedValue = 150;
int trimValue = 0;

bool invertLeft = false;
bool invertRight = false;

int currentLeftSpeed = 0;
int currentRightSpeed = 0;
int targetLeftSpeed = 0;
int targetRightSpeed = 0;

unsigned long lastRampTime = 0;
const int rampStep = 5;
const int rampInterval = 20;

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

void setJoystickMovement(int x, int y) {
  enableDrivers();

  x = constrain(x, -100, 100);
  y = constrain(y, -100, 100);

  int deadZone = 8;

  if (abs(x) < deadZone) x = 0;
  if (abs(y) < deadZone) y = 0;

  if (y != 0) {
    x = x + trimValue;
    x = constrain(x, -100, 100);
  }

  int steerX = x;
  if (y < 0) steerX = -x;

  int leftMix = y + steerX;
  int rightMix = y - steerX;

  int maxMix = max(abs(leftMix), abs(rightMix));
  if (maxMix > 100) {
    leftMix = (leftMix * 100) / maxMix;
    rightMix = (rightMix * 100) / maxMix;
  }

  targetLeftSpeed = (leftMix * speedValue) / 100;
  targetRightSpeed = (rightMix * speedValue) / 100;

  targetLeftSpeed = constrain(targetLeftSpeed, -255, 255);
  targetRightSpeed = constrain(targetRightSpeed, -255, 255);
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
  padding:30px 14px 22px;
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
  font-size:18px;
  font-weight:900;
  color:#222;
  letter-spacing:0.7px;
}

.trimBox {
  margin-top:18px;
  width:185px;
  height:32px;
  padding:6px 12px;
  border-radius:22px;
  background:linear-gradient(145deg,#ffffff,#eeeeee);
  box-shadow:
    5px 5px 12px rgba(0,0,0,0.18),
    -5px -5px 12px rgba(255,255,255,0.95),
    inset 2px 2px 5px rgba(255,255,255,0.85),
    inset -2px -2px 5px rgba(0,0,0,0.08);
  display:flex;
  align-items:center;
  justify-content:center;
}

#trimSlider {
  width:155px;
  height:6px;
  appearance:none;
  border-radius:20px;
  background:linear-gradient(90deg,#dddddd 0%, #dddddd 50%, #111 50%, #111 50%, #dddddd 50%, #dddddd 100%);
  box-shadow:
    inset 2px 2px 5px rgba(0,0,0,0.20),
    inset -2px -2px 5px rgba(255,255,255,0.95);
}

#trimSlider::-webkit-slider-thumb {
  appearance:none;
  width:23px;
  height:23px;
  border-radius:50%;
  background:
    radial-gradient(circle at 30% 25%,#ffffff,#d9d9d9 35%,#444 78%,#111);
  box-shadow:
    0 5px 9px rgba(0,0,0,0.35),
    inset 3px 3px 6px rgba(255,255,255,0.55),
    inset -3px -3px 6px rgba(0,0,0,0.35);
}

.joyWrap {
  margin-top:25px;
  width:315px;
  height:315px;
  border-radius:50%;
  background:
    radial-gradient(circle at 35% 25%, rgba(255,255,255,0.95), rgba(245,245,245,0.85) 28%, rgba(190,190,190,0.45) 60%, rgba(60,60,60,0.18) 100%),
    linear-gradient(145deg,#fdfdfd,#d7d7d7);
  box-shadow:
    18px 18px 35px rgba(0,0,0,0.35),
    -12px -12px 28px rgba(255,255,255,0.95),
    inset 8px 8px 18px rgba(255,255,255,0.8),
    inset -12px -12px 25px rgba(0,0,0,0.16);
  display:flex;
  justify-content:center;
  align-items:center;
  position:relative;
  touch-action:none;
}

.joyWrap::before {
  content:"";
  position:absolute;
  width:245px;
  height:245px;
  border-radius:50%;
  background:
    radial-gradient(circle at 50% 50%, rgba(230,230,230,0.5), rgba(120,120,120,0.18) 60%, rgba(0,0,0,0.13)),
    linear-gradient(145deg,#eeeeee,#ffffff);
  box-shadow:
    inset 10px 10px 22px rgba(0,0,0,0.13),
    inset -10px -10px 22px rgba(255,255,255,0.95);
}

.joyBase {
  width:230px;
  height:230px;
  border-radius:50%;
  position:relative;
  background:
    radial-gradient(circle at center, rgba(0,120,255,0.07), transparent 38%),
    radial-gradient(circle at center, transparent 0 60%, rgba(0,0,0,0.12) 61%, transparent 64%);
  z-index:2;
}

.joyBase::before,
.joyBase::after {
  content:"";
  position:absolute;
  background:rgba(0,0,0,0.13);
  border-radius:20px;
}

.joyBase::before {
  width:160px;
  height:5px;
  left:35px;
  top:112px;
}

.joyBase::after {
  width:5px;
  height:160px;
  left:112px;
  top:35px;
}

.knob {
  width:105px;
  height:105px;
  border-radius:50%;
  position:absolute;
  left:62.5px;
  top:62.5px;
  background:
    radial-gradient(circle at 30% 22%, #ffffff, #bfe6ff 20%, #268fff 48%, #064bbd 78%, #021c56 100%);
  box-shadow:
    0 22px 22px rgba(0,0,0,0.35),
    inset 8px 8px 14px rgba(255,255,255,0.55),
    inset -10px -12px 18px rgba(0,0,0,0.42),
    0 0 24px rgba(0,120,255,0.4);
  z-index:5;
  transition:box-shadow 0.12s;
}

.knob::before {
  content:"";
  position:absolute;
  width:52px;
  height:26px;
  border-radius:50%;
  left:20px;
  top:14px;
  background:rgba(255,255,255,0.45);
  filter:blur(1px);
  transform:rotate(-20deg);
}

.knob.active {
  box-shadow:
    0 26px 28px rgba(0,0,0,0.42),
    inset 8px 8px 14px rgba(255,255,255,0.55),
    inset -10px -12px 18px rgba(0,0,0,0.42),
    0 0 35px rgba(0,130,255,0.8);
}

.joyText {
  position:absolute;
  bottom:16px;
  width:210px;
  height:58px;
  z-index:6;
  pointer-events:none;
}

.joyText svg {
  width:100%;
  height:100%;
  overflow:visible;
}

.joyText text {
  font-size:9px;
  font-weight:900;
  letter-spacing:1.3px;
  fill:#111;
  opacity:0.78;
  text-shadow:
    0 1px 0 rgba(255,255,255,0.9),
    0 3px 5px rgba(0,0,0,0.35);
}

.speedBox {
  margin-top:28px;
  width:325px;
  min-height:112px;
  padding:18px 18px 16px;
  border-radius:34px;
  background:
    linear-gradient(145deg,#ffffff,#eeeeee);
  box-shadow:
    10px 10px 22px rgba(0,0,0,0.25),
    -8px -8px 20px rgba(255,255,255,0.98),
    inset 3px 3px 8px rgba(255,255,255,0.85),
    inset -4px -4px 10px rgba(0,0,0,0.08);
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
}

.speedBox label {
  font-size:26px;
  font-weight:900;
  margin-bottom:14px;
  padding:5px 18px;
  border-radius:22px;
  background:linear-gradient(145deg,#ffffff,#e5e5e5);
  box-shadow:
    5px 5px 10px rgba(0,0,0,0.18),
    -5px -5px 10px rgba(255,255,255,0.95);
  color:#111;
  text-shadow:
    0 1px 0 #fff,
    0 3px 5px rgba(0,0,0,0.32),
    0 8px 13px rgba(0,0,0,0.18);
}

.speedRow {
  display:flex;
  align-items:center;
  gap:13px;
}

input[type=range] {
  width:215px;
  height:10px;
  appearance:none;
  border-radius:20px;
  background:linear-gradient(90deg,#0b8cff 0%, #0b8cff 59%, #e5e5e5 59%, #e5e5e5 100%);
  box-shadow:
    inset 3px 3px 7px rgba(0,0,0,0.2),
    inset -3px -3px 7px rgba(255,255,255,0.95);
}

input[type=range]::-webkit-slider-thumb {
  appearance:none;
  width:39px;
  height:39px;
  border-radius:50%;
  background:
    radial-gradient(circle at 30% 25%,#ffffff,#8ed5ff 35%,#0077ff 75%,#003d99);
  box-shadow:
    0 8px 16px rgba(0,0,0,0.28),
    inset 4px 4px 8px rgba(255,255,255,0.55),
    inset -4px -4px 8px rgba(0,0,0,0.25);
}

#speedVal {
  font-size:31px;
  font-weight:900;
  width:58px;
  text-align:left;
  color:#111;
  text-shadow:
    0 1px 0 #ffffff,
    0 3px 4px rgba(0,0,0,0.42),
    0 8px 12px rgba(0,0,0,0.22);
}

@media (orientation: landscape) {
  .main {
    min-height:100dvh;
    height:auto;
    padding:12px 22px 18px;
    display:grid;
    grid-template-columns:1fr 350px;
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

  .trimBox {
    grid-column:1;
    grid-row:3;
    align-self:start;
    margin-top:0;
    transform:translateY(-8px);
  }

  .joyWrap {
    grid-column:1;
    grid-row:3;
    margin-top:36px;
    transform:scale(0.78);
  }

  .speedBox {
    grid-column:2;
    grid-row:3;
    margin-top:0;
    width:325px;
  }
}

@media (orientation: landscape) and (max-height:390px) {
  h1 { font-size:24px; }
  h3 { font-size:12px; }

  .joyWrap {
    transform:scale(0.72);
  }

  .speedBox {
    transform:scale(0.92);
  }
}
</style>
</head>

<body>
<div class="main">
  <h1>ESP32-6WD</h1>
  <h3>Joystick Control</h3>

  <div class="trimBox">
    <input id="trimSlider" type="range" min="-25" max="25" value="0" oninput="setTrim(this.value)">
  </div>

  <div id="joyWrap" class="joyWrap">
    <div id="joyBase" class="joyBase">
      <div id="knob" class="knob"></div>
    </div>

    <div class="joyText">
      <svg viewBox="0 0 210 58">
        <path id="uTextPath" d="M 20 0 Q 105 65 199 0" fill="none"/>
        <text>
          <textPath href="#uTextPath" startOffset="50%" text-anchor="middle">
            360° SMOOTH DRIVE
          </textPath>
        </text>
      </svg>
    </div>
  </div>

  <div class="speedBox">
    <label id="speedLabel">Speed</label>
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

const joyBase = document.getElementById('joyBase');
const knob = document.getElementById('knob');

let active = false;
let joyX = 0;
let joyY = 0;
let lastSend = 0;

const maxMove = 62;

function sendJoy(x, y, force=false) {
  const now = Date.now();
  if (!force && now - lastSend < 45) return;
  lastSend = now;

  fetch('/joy?x=' + Math.round(x) + '&y=' + Math.round(y)).catch(()=>{});
}

function moveKnob(clientX, clientY) {
  const rect = joyBase.getBoundingClientRect();
  const cx = rect.left + rect.width / 2;
  const cy = rect.top + rect.height / 2;

  let dx = clientX - cx;
  let dy = clientY - cy;

  const dist = Math.sqrt(dx * dx + dy * dy);

  if (dist > maxMove) {
    dx = dx / dist * maxMove;
    dy = dy / dist * maxMove;
  }

  knob.style.transform = 'translate(' + dx + 'px,' + dy + 'px)';
  knob.classList.add('active');

  joyX = (dx / maxMove) * 100;
  joyY = (-dy / maxMove) * 100;

  sendJoy(joyX, joyY);
}

joyBase.addEventListener('pointerdown', e => {
  active = true;
  joyBase.setPointerCapture(e.pointerId);
  moveKnob(e.clientX, e.clientY);
});

joyBase.addEventListener('pointermove', e => {
  if (!active) return;
  moveKnob(e.clientX, e.clientY);
});

function releaseJoy() {
  active = false;
  joyX = 0;
  joyY = 0;
  knob.style.transform = 'translate(0px,0px)';
  knob.classList.remove('active');
  sendJoy(0, 0, true);
}

joyBase.addEventListener('pointerup', releaseJoy);
joyBase.addEventListener('pointercancel', releaseJoy);
joyBase.addEventListener('lostpointercapture', releaseJoy);

function getSpeedColor(percent) {
  let hue = 210 - (percent * 2.1);
  return "hsl(" + hue + ", 100%, 45%)";
}

function setTrim(v) {
  let val = Number(v);
  let percent = ((val + 25) / 50) * 100;

  if (val >= 0) {
    document.getElementById('trimSlider').style.background =
      "linear-gradient(90deg, #dddddd 0%, #dddddd 50%, #111 50%, #111 " + percent + "%, #dddddd " + percent + "%, #dddddd 100%)";
  } else {
    document.getElementById('trimSlider').style.background =
      "linear-gradient(90deg, #dddddd 0%, #dddddd " + percent + "%, #111 " + percent + "%, #111 50%, #dddddd 50%, #dddddd 100%)";
  }

  fetch('/trim?value=' + val).catch(()=>{});
}

function setSpeed(v) {
  let percent = Math.round((v / 255) * 100);
  let color = getSpeedColor(percent);

  document.getElementById('speedVal').innerHTML = percent + "%";

  document.getElementById('speedSlider').style.background =
    "linear-gradient(90deg, " + color + " 0%, " + color + " " + percent + "%, #e5e5e5 " + percent + "%, #e5e5e5 100%)";

  fetch('/speed?value=' + v).catch(()=>{});
}

setTrim(0);
setSpeed(150);
</script>

</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", page);
}

void handleJoy() {
  int x = 0;
  int y = 0;

  if (server.hasArg("x")) x = server.arg("x").toInt();
  if (server.hasArg("y")) y = server.arg("y").toInt();

  setJoystickMovement(x, y);

  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    speedValue = server.arg("value").toInt();
    speedValue = constrain(speedValue, 0, 255);
  }

  server.send(200, "text/plain", "OK");
}

void handleTrim() {
  if (server.hasArg("value")) {
    trimValue = server.arg("value").toInt();
    trimValue = constrain(trimValue, -25, 25);
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
  setJoystickMovement(0, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  esp_wifi_set_max_tx_power(78);

  server.on("/", handleRoot);
  server.on("/joy", handleJoy);
  server.on("/speed", handleSpeed);
  server.on("/trim", handleTrim);

  server.begin();
}

void loop() {
  server.handleClient();
  updateMotorsSmooth();
}