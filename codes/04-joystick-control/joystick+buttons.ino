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
int trimValue = 0;

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
char activeControlMode = 'B';  // B = buttons, J = joystick
int lastJoyX = 0;
int lastJoyY = 0;

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

void applyTrimToTargets() {
  if (driveMode == 'S' && steerMode == 'N') return;

  int correction = (speedValue * trimValue) / 100;

  if (driveMode == 'B') correction = -correction;

  targetLeftSpeed += correction;
  targetRightSpeed -= correction;

  targetLeftSpeed = constrain(targetLeftSpeed, -255, 255);
  targetRightSpeed = constrain(targetRightSpeed, -255, 255);
}

void applyButtonMovement() {
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

  applyTrimToTargets();
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

void stopCarNow() {
  driveMode = 'S';
  steerMode = 'N';
  targetLeftSpeed = 0;
  targetRightSpeed = 0;
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

String mainPage = R"rawliteral(
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

html,body{
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

.main{
  width:100vw;
  min-height:100dvh;
  padding:26px 14px 18px;
  display:flex;
  flex-direction:column;
  align-items:center;
}

h1{
  margin:0;
  font-size:35px;
  font-weight:900;
  color:#111;
  text-shadow:0 6px 6px rgba(0,0,0,0.50);
}

h3{
  margin:8px 0 0;
  font-size:17px;
  font-weight:900;
  color:#222;
  letter-spacing:0.7px;
}

.trimBox{
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

#trimSlider{
  width:155px;
  height:6px;
  appearance:none;
  border-radius:20px;
  background:linear-gradient(90deg,#dddddd 0%, #dddddd 50%, #111 50%, #111 50%, #dddddd 50%, #dddddd 100%);
  box-shadow:
    inset 2px 2px 5px rgba(0,0,0,0.20),
    inset -2px -2px 5px rgba(255,255,255,0.95);
}

#trimSlider::-webkit-slider-thumb{
  appearance:none;
  width:23px;
  height:23px;
  border-radius:50%;
  background:radial-gradient(circle at 30% 25%,#ffffff,#d9d9d9 35%,#444 78%,#111);
  box-shadow:
    0 5px 9px rgba(0,0,0,0.35),
    inset 3px 3px 6px rgba(255,255,255,0.55),
    inset -3px -3px 6px rgba(0,0,0,0.35);
}

.controlArea{
  margin-top:25px;
  width:315px;
  height:315px;
  display:flex;
  align-items:center;
  justify-content:center;
}

.controls{
  position:relative;
  width:305px;
  height:305px;
  display:none;
}

.btn{
  position:absolute;
  width:82px;
  height:82px;
  border:none;
  border-radius:24px;
  background:
    radial-gradient(circle at 30% 22%,rgba(255,255,255,0.98),rgba(255,255,255,0.55) 18%,transparent 38%),
    linear-gradient(145deg,#ffffff,#d8d8d8);
  box-shadow:
    10px 10px 22px rgba(0,0,0,0.34),
    -8px -8px 20px rgba(255,255,255,0.98),
    inset 4px 4px 8px rgba(255,255,255,0.88),
    inset -5px -5px 10px rgba(0,0,0,0.12);
  display:flex;
  justify-content:center;
  align-items:center;
  transition:transform 0.08s ease, box-shadow 0.08s ease, background 0.08s ease;
}

.btn:active,.btn.active{
  transform:translateY(6px) scale(0.96);
  background:
    radial-gradient(circle at 30% 22%,#ffffff,#91dcff 22%,transparent 42%),
    linear-gradient(145deg,#00d4ff,#0077ff);
  box-shadow:
    0 0 0 5px rgba(0,140,255,0.22),
    0 0 26px rgba(0,140,255,0.72),
    inset 6px 6px 13px rgba(0,0,0,0.24),
    inset -5px -5px 12px rgba(255,255,255,0.38);
}

.btn.active .up{border-bottom-color:white;}
.btn.active .down{border-top-color:white;}
.btn:active .leftA{border-right-color:white;}
.btn:active .rightA{border-left-color:white;}

.stop{
  width:102px;
  height:102px;
  border-radius:50%;
  left:101px;
  top:102px;
  background:radial-gradient(circle at 30% 23%,#ffabab,#ff1e1e 45%,#b00000 72%,#570000);
  color:white;
  font-size:14px;
  font-weight:900;
  letter-spacing:1.5px;
  text-shadow:0 3px 5px rgba(0,0,0,0.75);
  box-shadow:
    0 14px 0 #650000,
    0 24px 28px rgba(0,0,0,0.32),
    inset 5px 5px 12px rgba(255,255,255,0.35),
    inset -7px -7px 14px rgba(0,0,0,0.28);
}

.stop:active{
  transform:translateY(10px) scale(0.96);
  box-shadow:
    0 5px 0 #650000,
    0 14px 18px rgba(0,0,0,0.25),
    inset 6px 6px 14px rgba(0,0,0,0.20);
}

.forward{left:111px;top:0;}
.left{left:0;top:111px;}
.right{right:0;top:111px;}
.back{left:111px;top:222px;}

.arrow{width:0;height:0;filter:drop-shadow(0 3px 2px rgba(0,0,0,0.25));}
.up{border-left:17px solid transparent;border-right:17px solid transparent;border-bottom:29px solid #111;}
.down{border-left:17px solid transparent;border-right:17px solid transparent;border-top:29px solid #111;}
.leftA{border-top:17px solid transparent;border-bottom:17px solid transparent;border-right:29px solid #111;}
.rightA{border-top:17px solid transparent;border-bottom:17px solid transparent;border-left:29px solid #111;}

.joyWrap{
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

.joyWrap::before{
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

.joyBase{
  width:230px;
  height:230px;
  border-radius:50%;
  position:relative;
  background:
    radial-gradient(circle at center, rgba(0,120,255,0.07), transparent 38%),
    radial-gradient(circle at center, transparent 0 60%, rgba(0,0,0,0.12) 61%, transparent 64%);
  z-index:2;
}

.joyBase::before,.joyBase::after{
  content:"";
  position:absolute;
  background:rgba(0,0,0,0.13);
  border-radius:20px;
}
.joyBase::before{width:160px;height:5px;left:35px;top:112px;}
.joyBase::after{width:5px;height:160px;left:112px;top:35px;}

.knob{
  width:105px;
  height:105px;
  border-radius:50%;
  position:absolute;
  left:62.5px;
  top:62.5px;
  background:radial-gradient(circle at 30% 22%, #ffffff, #bfe6ff 20%, #268fff 48%, #064bbd 78%, #021c56 100%);
  box-shadow:
    0 22px 22px rgba(0,0,0,0.35),
    inset 8px 8px 14px rgba(255,255,255,0.55),
    inset -10px -12px 18px rgba(0,0,0,0.42),
    0 0 24px rgba(0,120,255,0.4);
  z-index:5;
  transition:box-shadow 0.12s;
}

.knob::before{
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

.knob.active{
  box-shadow:
    0 26px 28px rgba(0,0,0,0.42),
    inset 8px 8px 14px rgba(255,255,255,0.55),
    inset -10px -12px 18px rgba(0,0,0,0.42),
    0 0 35px rgba(0,130,255,0.8);
}

.joyText{
  position:absolute;
  bottom:16px;
  width:210px;
  height:58px;
  z-index:6;
  pointer-events:none;
}
.joyText svg{width:100%;height:100%;overflow:visible;}
.joyText text{
  font-size:9px;
  font-weight:900;
  letter-spacing:1.3px;
  fill:#111;
  opacity:0.78;
}

.speedBox{
  margin-top:28px;
  width:325px;
  min-height:112px;
  padding:18px 18px 16px;
  border-radius:34px;
  background:linear-gradient(145deg,#ffffff,#eeeeee);
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

.speedBox label{
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
  text-shadow:0 1px 0 #fff,0 3px 5px rgba(0,0,0,0.32),0 8px 13px rgba(0,0,0,0.18);
}

.speedRow{
  display:flex;
  align-items:center;
  gap:13px;
}

input[type=range]{
  width:215px;
  height:10px;
  appearance:none;
  border-radius:20px;
  background:linear-gradient(90deg,#0b8cff 0%, #0b8cff 59%, #e5e5e5 59%, #e5e5e5 100%);
  box-shadow:inset 3px 3px 7px rgba(0,0,0,0.2),inset -3px -3px 7px rgba(255,255,255,0.95);
}

input[type=range]::-webkit-slider-thumb{
  appearance:none;
  width:39px;
  height:39px;
  border-radius:50%;
  background:radial-gradient(circle at 30% 25%,#ffffff,#8ed5ff 35%,#0077ff 75%,#003d99);
  box-shadow:
    0 8px 16px rgba(0,0,0,0.28),
    inset 4px 4px 8px rgba(255,255,255,0.55),
    inset -4px -4px 8px rgba(0,0,0,0.25);
}

#speedVal{
  font-size:20px;
  font-weight:900;
  width:48px;
  text-align:left;
  color:#111;
  text-shadow:0 1px 0 #ffffff,0 2px 4px rgba(0,0,0,0.42);
}

.modeSwitchBox{
  margin-top:18px;
  display:flex;
  align-items:center;
  gap:10px;
  padding:8px 11px;
  border-radius:50px;
  background:linear-gradient(145deg,rgba(255,255,255,0.96),rgba(230,230,230,0.92));
  box-shadow:
    7px 7px 18px rgba(0,0,0,0.26),
    -5px -5px 14px rgba(255,255,255,0.95),
    inset 2px 2px 5px rgba(255,255,255,0.8),
    inset -2px -2px 5px rgba(0,0,0,0.10);
}

.modeText{
  width:62px;
  font-size:12px;
  line-height:13px;
  font-weight:900;
  text-align:center;
  color:#111;
  text-shadow:0 1px 0 #fff,0 2px 4px rgba(0,0,0,0.22);
}

.switch{
  width:82px;
  height:40px;
  border-radius:40px;
  border:none;
  position:relative;
  background:linear-gradient(145deg,#fdfdfd,#d8d8d8);
  box-shadow:
    inset 5px 5px 10px rgba(0,0,0,0.20),
    inset -5px -5px 10px rgba(255,255,255,0.95),
    0 5px 12px rgba(0,0,0,0.25);
  padding:0;
}

.knobSwitch{
  position:absolute;
  top:4px;
  left:4px;
  width:32px;
  height:32px;
  border-radius:50%;
  background:radial-gradient(circle at 30% 25%,#ffffff,#bfe6ff 25%,#248fff 58%,#0345a8 100%);
  box-shadow:0 6px 10px rgba(0,0,0,0.35),inset 3px 3px 6px rgba(255,255,255,0.65),inset -4px -4px 7px rgba(0,0,0,0.30);
  transition:left 0.22s ease;
}

.switch.joy .knobSwitch{left:46px;}

@media (orientation:landscape){
  .main{
    min-height:100dvh;
    height:auto;
    padding:10px 18px 12px;
    display:grid;
    grid-template-columns:1fr 350px;
    grid-template-rows:auto auto 1fr auto;
    column-gap:18px;
    row-gap:4px;
    align-items:center;
    justify-items:center;
  }
  h1{grid-column:1/-1;font-size:27px;line-height:1;}
  h3{grid-column:1/-1;font-size:13px;margin:0;}
  .trimBox{grid-column:1;grid-row:3;align-self:start;margin-top:0;transform:translateY(-7px);}
  .controlArea{grid-column:1;grid-row:3;margin-top:34px;transform:scale(0.76);}
  .speedBox{grid-column:2;grid-row:3;margin-top:0;width:325px;}
  .modeSwitchBox{grid-column:1/-1;grid-row:4;margin-top:2px;transform:scale(0.92);}
}

@media (orientation:landscape) and (max-height:390px){
  h1{font-size:23px;}
  h3{font-size:12px;}
  .controlArea{transform:scale(0.70);}
  .speedBox{transform:scale(0.90);}
  .modeSwitchBox{transform:scale(0.86);}
}
</style>
</head>

<body>
<div class="main">
  <h1>ESP32-6WD</h1>
  <h3 id="modeTitle">Button Control</h3>

  <div class="trimBox">
    <input id="trimSlider" type="range" min="-25" max="25" value="0" oninput="setTrim(this.value)">
  </div>

  <div class="controlArea">
    <div id="buttonControls" class="controls">
      <button id="forwardBtn" class="btn forward" onclick="toggleDrive('F')"><div class="arrow up"></div></button>
      <button class="btn left" onpointerdown="steer('L')" onpointerup="steer('N')" onpointercancel="steer('N')" onpointerleave="steer('N')"><div class="arrow leftA"></div></button>
      <button class="btn stop" onpointerdown="stopCar()">STOP</button>
      <button class="btn right" onpointerdown="steer('R')" onpointerup="steer('N')" onpointercancel="steer('N')" onpointerleave="steer('N')"><div class="arrow rightA"></div></button>
      <button id="backBtn" class="btn back" onclick="toggleDrive('B')"><div class="arrow down"></div></button>
    </div>

    <div id="joyWrap" class="joyWrap">
      <div id="joyBase" class="joyBase"><div id="knob" class="knob"></div></div>
      <div class="joyText">
        <svg viewBox="0 0 210 58">
          <path id="uTextPath" d="M 20 0 Q 105 65 199 0" fill="none"/>
          <text><textPath href="#uTextPath" startOffset="50%" text-anchor="middle">360° SMOOTH DRIVE</textPath></text>
        </svg>
      </div>
    </div>
  </div>

  <div class="speedBox">
    <label>Speed</label>
    <div class="speedRow">
      <input id="speedSlider" type="range" min="0" max="255" value="150" oninput="setSpeed(this.value)">
      <div id="speedVal">59%</div>
    </div>
  </div>

  <div class="modeSwitchBox">
    <div class="modeText">Button<br>Mode</div>
    <button id="modeSwitch" class="switch" onclick="toggleMode()"><span class="knobSwitch"></span></button>
    <div class="modeText">Joystick<br>Mode</div>
  </div>
</div>

<script>
document.addEventListener('contextmenu', e => e.preventDefault());
document.addEventListener('selectstart', e => e.preventDefault());
document.addEventListener('copy', e => e.preventDefault());
document.addEventListener('cut', e => e.preventDefault());
document.addEventListener('dragstart', e => e.preventDefault());

let mode = 'buttons';
let driveMode = 'S';
let active = false;
let joyX = 0;
let joyY = 0;
let lastSend = 0;
const maxMove = 62;

const buttonControls = document.getElementById('buttonControls');
const joyWrap = document.getElementById('joyWrap');
const joyBase = document.getElementById('joyBase');
const knob = document.getElementById('knob');

function send(c){ fetch('/cmd?move=' + c).catch(()=>{}); }
function sendJoy(x,y,force=false){
  const now = Date.now();
  if(!force && now - lastSend < 45) return;
  lastSend = now;
  fetch('/joy?x=' + Math.round(x) + '&y=' + Math.round(y)).catch(()=>{});
}

function stopAll(){
  driveMode = 'S';
  send('S');
  sendJoy(0,0,true);
  updateButtons();
  releaseJoy(false);
}

function showMode(){
  const sw = document.getElementById('modeSwitch');
  const title = document.getElementById('modeTitle');
  if(mode === 'buttons'){
    buttonControls.style.display = 'block';
    joyWrap.style.display = 'none';
    sw.classList.remove('joy');
    title.innerHTML = 'Button Control';
  }else{
    buttonControls.style.display = 'none';
    joyWrap.style.display = 'flex';
    sw.classList.add('joy');
    title.innerHTML = 'Joystick Control';
  }
}

function toggleMode(){
  stopAll();
  mode = (mode === 'buttons') ? 'joystick' : 'buttons';
  showMode();
}

function updateButtons(){
  document.getElementById('forwardBtn').classList.toggle('active', driveMode === 'F');
  document.getElementById('backBtn').classList.toggle('active', driveMode === 'B');
}

function toggleDrive(m){
  if(mode !== 'buttons') return;
  if(driveMode === m){ driveMode = 'S'; send('S'); }
  else{ driveMode = m; send(m); }
  updateButtons();
}

function steer(m){
  if(mode !== 'buttons') return;
  send(m);
}

function stopCar(){
  driveMode = 'S';
  send('S');
  updateButtons();
}

function moveKnob(clientX,clientY){
  const rect = joyBase.getBoundingClientRect();
  const cx = rect.left + rect.width / 2;
  const cy = rect.top + rect.height / 2;
  let dx = clientX - cx;
  let dy = clientY - cy;
  const dist = Math.sqrt(dx * dx + dy * dy);
  if(dist > maxMove){ dx = dx / dist * maxMove; dy = dy / dist * maxMove; }
  knob.style.transform = 'translate(' + dx + 'px,' + dy + 'px)';
  knob.classList.add('active');
  joyX = (dx / maxMove) * 100;
  joyY = (-dy / maxMove) * 100;
  sendJoy(joyX, joyY);
}

joyBase.addEventListener('pointerdown', e => {
  if(mode !== 'joystick') return;
  active = true;
  joyBase.setPointerCapture(e.pointerId);
  moveKnob(e.clientX, e.clientY);
});
joyBase.addEventListener('pointermove', e => { if(active) moveKnob(e.clientX, e.clientY); });

function releaseJoy(sendCmd=true){
  active = false;
  joyX = 0;
  joyY = 0;
  knob.style.transform = 'translate(0px,0px)';
  knob.classList.remove('active');
  if(sendCmd) sendJoy(0,0,true);
}

joyBase.addEventListener('pointerup', () => releaseJoy(true));
joyBase.addEventListener('pointercancel', () => releaseJoy(true));
joyBase.addEventListener('lostpointercapture', () => releaseJoy(true));

function getSpeedColor(percent){
  let hue = 210 - (percent * 2.1);
  return 'hsl(' + hue + ', 100%, 45%)';
}

function setTrim(v){
  let val = Number(v);
  let percent = ((val + 25) / 50) * 100;
  if(val >= 0){
    document.getElementById('trimSlider').style.background =
      'linear-gradient(90deg, #dddddd 0%, #dddddd 50%, #111 50%, #111 ' + percent + '%, #dddddd ' + percent + '%, #dddddd 100%)';
  }else{
    document.getElementById('trimSlider').style.background =
      'linear-gradient(90deg, #dddddd 0%, #dddddd ' + percent + '%, #111 ' + percent + '%, #111 50%, #dddddd 50%, #dddddd 100%)';
  }
  fetch('/trim?value=' + val).catch(()=>{});
}

function setSpeed(v){
  let percent = Math.round((v / 255) * 100);
  let color = getSpeedColor(percent);
  document.getElementById('speedVal').innerHTML = percent + '%';
  document.getElementById('speedSlider').style.background =
    'linear-gradient(90deg, ' + color + ' 0%, ' + color + ' ' + percent + '%, #e5e5e5 ' + percent + '%, #e5e5e5 100%)';
  fetch('/speed?value=' + v).catch(()=>{});
}

showMode();
setTrim(0);
setSpeed(150);
updateButtons();
</script>
</body>
</html>
)rawliteral";

String buttonsPage = R"rawliteral(







<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>ESP32-6WD</title>

<style>
*{
  user-select:none !important;
  -webkit-user-select:none !important;
  -moz-user-select:none !important;
  -ms-user-select:none !important;
  -webkit-touch-callout:none !important;
  -webkit-tap-highlight-color:transparent;
  box-sizing:border-box;
}

button, input, label, div, span, h1, h3, svg, text {
  user-select:none !important;
  -webkit-user-select:none !important;
  -webkit-touch-callout:none !important;
}

html, body {
  margin:0;
  padding:0;
  width:100%;
  min-height:100%;
  overflow-x:hidden;
  overflow-y:auto;
  background:#fff;
font-family:'SF Pro Display','-apple-system','BlinkMacSystemFont','Segoe UI','Roboto',Arial,sans-serif;
  overscroll-behavior:contain;
}

.main {
  width:100vw;
  min-height:100dvh;
  padding:28px 14px 22px;
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
  margin-top:16px;
  width:168px;
  height:29px;
  padding:5px 11px;
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
  width:140px;
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
  width:21px;
  height:21px;
  border-radius:50%;
  background:
    radial-gradient(circle at 30% 25%,#ffffff,#d9d9d9 35%,#444 78%,#111);
  box-shadow:
    0 5px 9px rgba(0,0,0,0.35),
    inset 3px 3px 6px rgba(255,255,255,0.55),
    inset -3px -3px 6px rgba(0,0,0,0.35);
}

.modeStage {
  margin-top:20px;
  position:relative;
  touch-action:pan-y;
  isolation:isolate;
}

.modeTrack {
  width:200%;
  height:100%;
  display:flex;
  transition:transform 0.32s cubic-bezier(.2,.9,.25,1);
}

.modeStage.joystickMode .modeTrack {
  transform:translateX(-50%);
}

.modePanel {
  width:50%;
  height:100%;
  flex:0 0 50%;
  display:flex;
  justify-content:center;
  align-items:center;
}

.controls {
  position:relative;
  width:315px;
  height:315px;
  border-radius:50%;
  background:
    radial-gradient(circle at 35% 25%, rgba(255,255,255,0.96), rgba(245,245,245,0.86) 28%, rgba(190,190,190,0.42) 60%, rgba(60,60,60,0.17) 100%),
    linear-gradient(145deg,#fdfdfd,#d7d7d7);
  box-shadow:
    18px 18px 35px rgba(0,0,0,0.32),
    -12px -12px 28px rgba(255,255,255,0.95),
    inset 8px 8px 18px rgba(255,255,255,0.82),
    inset -12px -12px 25px rgba(0,0,0,0.15);
}

.controls::before {
  content:"";
  position:absolute;
  width:245px;
  height:245px;
  left:35px;
  top:35px;
  border-radius:50%;
  background:
    radial-gradient(circle at 50% 50%, rgba(230,230,230,0.45), rgba(120,120,120,0.16) 60%, rgba(0,0,0,0.12)),
    linear-gradient(145deg,#eeeeee,#ffffff);
  box-shadow:
    inset 10px 10px 22px rgba(0,0,0,0.13),
    inset -10px -10px 22px rgba(255,255,255,0.95);
}

.btn {
  position:absolute;
  width:76px;
  height:76px;
  border:none;
  outline:none;
  border-radius:50%;
  background:
    radial-gradient(circle at 30% 20%, rgba(255,255,255,0.98) 0 12%, rgba(174,225,255,0.98) 22%, rgba(38,143,255,0.98) 48%, rgba(6,75,189,0.98) 77%, rgba(2,28,86,0.98) 100%);
  box-shadow:
    inset 8px 8px 15px rgba(255,255,255,0.58),
    inset -10px -12px 18px rgba(0,0,0,0.43),
    0 18px 18px rgba(0,0,0,0.30),
    0 0 24px rgba(0,120,255,0.28);
  display:flex;
  justify-content:center;
  align-items:center;
  z-index:4;
  overflow:hidden;
  cursor:pointer;
  transform:translateZ(0);
  transition:transform 0.11s ease, box-shadow 0.11s ease, filter 0.11s ease, background 0.11s ease;
}

.btn::before {
  content:"";
  position:absolute;
  width:48px;
  height:25px;
  border-radius:50%;
  left:13px;
  top:8px;
  background:linear-gradient(180deg,rgba(255,255,255,0.70),rgba(255,255,255,0.08));
  filter:blur(0.4px);
  transform:rotate(-18deg);
  pointer-events:none;
}

.btn::after {
  content:"";
  position:absolute;
  inset:7px;
  border-radius:50%;
  border:1px solid rgba(255,255,255,0.28);
  box-shadow:inset 0 0 11px rgba(255,255,255,0.20);
  pointer-events:none;
}

.btn:active {
  transform:translateY(7px) scale(0.94);
  filter:saturate(1.15);
  box-shadow:
    inset 7px 8px 15px rgba(0,0,0,0.46),
    inset -5px -5px 12px rgba(255,255,255,0.36),
    0 7px 8px rgba(0,0,0,0.24),
    0 0 22px rgba(0,130,255,0.55);
}

.btn.active {
  transform:translateY(8px) scale(0.93);
  background:
    radial-gradient(circle at 48% 62%, rgba(185,235,255,1) 0 10%, rgba(0,212,255,1) 28%, rgba(0,119,255,1) 57%, rgba(0,50,150,1) 100%);
  filter:saturate(1.3) brightness(1.04);
  box-shadow:
    inset 9px 10px 17px rgba(0,0,0,0.50),
    inset -7px -7px 14px rgba(255,255,255,0.42),
    0 5px 7px rgba(0,0,0,0.26),
    0 0 0 5px rgba(0,140,255,0.16),
    0 0 34px rgba(0,145,255,0.78);
}

.btn.active::before {
  width:34px;
  height:16px;
  left:22px;
  top:27px;
  opacity:0.28;
}

.stop {
  width:104px;
  height:104px;
  border-radius:50%;
  left:105px;
  top:105px;
  background:
    radial-gradient(circle at 30% 20%,#ffffff 0 8%,#ffb4b4 18%,#ff3030 48%,#b40000 76%,#530000 100%);
  color:white;
  font-size:14px;
  font-weight:900;
  letter-spacing:1.5px;
  text-shadow:0 3px 5px rgba(0,0,0,0.75);
  box-shadow:
    inset 8px 8px 14px rgba(255,255,255,0.44),
    inset -12px -14px 20px rgba(0,0,0,0.50),
    0 20px 20px rgba(0,0,0,0.32),
    0 0 24px rgba(255,0,0,0.25);
}

.stop:active {
  transform:translateY(9px) scale(0.94);
  box-shadow:
    inset 9px 10px 17px rgba(0,0,0,0.52),
    inset -6px -6px 13px rgba(255,255,255,0.34),
    0 6px 9px rgba(0,0,0,0.25),
    0 0 25px rgba(255,0,0,0.55);
}

.forward { left:119px; top:16px; }
.left    { left:18px; top:120px; }
.right   { right:18px; top:120px; }
.back    { left:119px; top:223px; }

.arrow { width:0; height:0; filter:drop-shadow(0 3px 3px rgba(0,0,0,0.45)); z-index:2; }
.up {
  border-left:16px solid transparent;
  border-right:16px solid transparent;
  border-bottom:27px solid #fff;
}
.down {
  border-left:16px solid transparent;
  border-right:16px solid transparent;
  border-top:27px solid #fff;
}
.leftA {
  border-top:16px solid transparent;
  border-bottom:16px solid transparent;
  border-right:27px solid #fff;
}
.rightA {
  border-top:16px solid transparent;
  border-bottom:16px solid transparent;
  border-left:27px solid #fff;
}

.joyWrap {
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
.joyBase::before { width:160px; height:5px; left:35px; top:112px; }
.joyBase::after  { width:5px; height:160px; left:112px; top:35px; }

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
.joyText svg { width:100%; height:100%; overflow:visible; }
.joyText text {
  font-size:9px;
  font-weight:900;
  letter-spacing:1.3px;
  fill:#111;
  opacity:0.78;
  text-shadow:0 1px 0 rgba(255,255,255,0.9), 0 3px 5px rgba(0,0,0,0.35);
}

.modeDots {
  margin-top:3px;
  display:flex;
  gap:8px;
}
.modeDots span {
  width:11px;
  height:11px;
  border-radius:50%;
  background:#d7d7d7;
  box-shadow:inset 1px 1px 3px rgba(0,0,0,0.22), inset -1px -1px 3px rgba(255,255,255,0.9);
}
.modeStage.buttonMode ~ .modeDots .dotBtn,
.modeStage.joystickMode ~ .modeDots .dotJoy {
  background:#0b8cff;
  box-shadow:0 0 8px rgba(0,120,255,0.55);
}

.speedBox {
  margin-top:22px;
  width:325px;
  min-height:112px;
  padding:18px 18px 16px;
  border-radius:34px;
  background:linear-gradient(145deg,#ffffff,#eeeeee);
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
  font-size:20px;
  font-weight:400;
  margin-bottom:14px;
  padding:5px 18px;
  border-radius:22px;
  background:linear-gradient(145deg,#ffffff,#e5e5e5);
  box-shadow:
    5px 5px 10px rgba(0,0,0,0.18),
    -5px -5px 10px rgba(255,255,255,0.95);
  color:#111;
  text-shadow:0 1px 0 #fff, 0 3px 5px rgba(0,0,0,0.32), 0 8px 13px rgba(0,0,0,0.18);
}

.speedRow {
  display:flex;
  align-items:center;
  gap:13px;
}

input[type=range] {
  width:220px;
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
  font-size:17px;
  font-weight:900;
  width:45px;
  text-align:left;
  color:#111;
  text-shadow:0 1px 0 #ffffff, 0 3px 4px rgba(0,0,0,0.42), 0 8px 12px rgba(0,0,0,0.22);
}

@media (orientation: landscape) {
  .main {
    min-height:100dvh;
    height:auto;
    padding:10px 20px 16px;
    display:grid;
    grid-template-columns:1fr 350px;
    grid-template-rows:auto auto auto 1fr;
    column-gap:22px;
    row-gap:4px;
    align-items:center;
    justify-items:center;
  }

  h1 { grid-column:1 / -1; font-size:28px; line-height:1; }
  h3 { grid-column:1 / -1; font-size:14px; margin:0; }

  .trimBox {
    grid-column:1;
    grid-row:3;
    margin-top:2px;
  }

  .modeStage {
    grid-column:1;
    grid-row:4;
    margin-top:0;
    transform:scale(0.76);
    transform-origin:center;
  }

  .modeDots {
    grid-column:1;
    grid-row:4;
    align-self:end;
    margin-bottom:0;
  }

  .speedBox {
    grid-column:2;
    grid-row:3 / 5;
    margin-top:0;
    width:325px;
  }
}

@media (orientation: landscape) and (max-height:390px) {
  h1 { font-size:24px; }
  h3 { font-size:12px; }
  .trimBox { transform:scale(0.9); }
  .modeStage { transform:scale(0.69); }
  .speedBox { transform:scale(0.9); }
}
</style>
</head>

<body>
<div class="main">
  <h1>ESP32-6WD</h1>
  <h3 id="modeTitle">Button Control</h3>

  <div class="trimBox">
    <input id="trimSlider" type="range" min="-25" max="25" value="0" oninput="setTrim(this.value)">
  </div>

  <div id="modeStage" class="modeStage buttonMode">
    <div id="modeTrack" class="modeTrack">
      <div class="modePanel">
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
      </div>

      <div class="modePanel">
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
      </div>
    </div>
  </div>

  <div class="modeDots">
    <span class="dotBtn"></span>
    <span class="dotJoy"></span>
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
document.addEventListener('touchstart', e => {
  if (e.touches && e.touches.length > 1) e.preventDefault();
}, {passive:false});
document.addEventListener('gesturestart', e => e.preventDefault());
document.addEventListener('dblclick', e => e.preventDefault());
document.addEventListener('selectionchange', () => {
  const sel = window.getSelection && window.getSelection();
  if (sel && sel.rangeCount) sel.removeAllRanges();
});

const modeStage = document.getElementById('modeStage');
const modeTitle = document.getElementById('modeTitle');
const joyBase = document.getElementById('joyBase');
const knob = document.getElementById('knob');

let currentPanel = 0;
let swipeStartX = 0;
let swipeStartY = 0;
let driveMode = 'S';
let active = false;
let joyX = 0;
let joyY = 0;
let lastSend = 0;
const maxMove = 62;

function setPanel(panel) {
  currentPanel = panel;
  if (panel === 1) {
    modeStage.classList.remove('buttonMode');
    modeStage.classList.add('joystickMode');
    modeTitle.innerHTML = 'Joystick Control';
    stopCar();
  } else {
    modeStage.classList.remove('joystickMode');
    modeStage.classList.add('buttonMode');
    modeTitle.innerHTML = 'Button Control';
    releaseJoy();
  }
}

modeStage.addEventListener('pointerdown', e => {
  swipeStartX = e.clientX;
  swipeStartY = e.clientY;
});

modeStage.addEventListener('pointerup', e => {
  const dx = e.clientX - swipeStartX;
  const dy = e.clientY - swipeStartY;
  if (Math.abs(dx) > 55 && Math.abs(dx) > Math.abs(dy) * 1.4) {
    setPanel(currentPanel === 0 ? 1 : 0);
  }
});

function send(c) {
  fetch('/cmd?move=' + c).catch(()=>{});
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
updateButtons();
</script>

</body>
</html>





)rawliteral";

void handleRoot() {
  server.send(200, "text/html", mainPage);
}

void handleButtonsPage() {
  server.send(200, "text/html", buttonsPage);
}

void handleJoystickPage() {
  server.send(200, "text/html", mainPage);
}

void refreshCurrentModeMovement() {
  if (activeControlMode == 'J') {
    setJoystickMovement(lastJoyX, lastJoyY);
  } else {
    applyButtonMovement();
  }
}

void handleCmd() {
  if (!server.hasArg("move")) {
    server.send(400, "text/plain", "NO CMD");
    return;
  }

  String m = server.arg("move");
  activeControlMode = 'B';

  if (m == "F") {
    driveMode = 'F';
    steerMode = 'N';
    applyButtonMovement();
  }
  else if (m == "B") {
    driveMode = 'B';
    steerMode = 'N';
    applyButtonMovement();
  }
  else if (m == "L") {
    steerMode = 'L';
    applyButtonMovement();
  }
  else if (m == "R") {
    steerMode = 'R';
    applyButtonMovement();
  }
  else if (m == "N") {
    steerMode = 'N';
    applyButtonMovement();
  }
  else {
    stopCarNow();
  }

  server.send(200, "text/plain", "OK");
}

void handleJoy() {
  int x = 0;
  int y = 0;

  if (server.hasArg("x")) x = server.arg("x").toInt();
  if (server.hasArg("y")) y = server.arg("y").toInt();

  activeControlMode = 'J';
  lastJoyX = x;
  lastJoyY = y;
  driveMode = 'S';
  steerMode = 'N';
  setJoystickMovement(x, y);

  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    speedValue = server.arg("value").toInt();
    speedValue = constrain(speedValue, 0, 255);

    refreshCurrentModeMovement();
  }

  server.send(200, "text/plain", "OK");
}

void handleTrim() {
  if (server.hasArg("value")) {
    trimValue = server.arg("value").toInt();
    trimValue = constrain(trimValue, -25, 25);

    refreshCurrentModeMovement();
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
  stopCarNow();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  esp_wifi_set_max_tx_power(78);

  server.on("/", handleRoot);
  server.on("/buttons", handleButtonsPage);
  server.on("/joystick", handleJoystickPage);
  server.on("/cmd", handleCmd);
  server.on("/joy", handleJoy);
  server.on("/speed", handleSpeed);
  server.on("/trim", handleTrim);

  server.begin();
}

void loop() {
  server.handleClient();
  updateMotorsSmooth();
}
