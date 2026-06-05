#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

const char* ssid = "ESP32-6WD";
const char* password = "12345678";

WebServer server(80);

// ================= MOTOR PINS =================
#define LEFT_L_EN   32
#define LEFT_R_EN   33
#define LEFT_RPWM   19
#define LEFT_LPWM   18

#define RIGHT_L_EN  26
#define RIGHT_R_EN  25
#define RIGHT_RPWM  27
#define RIGHT_LPWM  14

#define BLUE_LED 2

// ================= EXTRA OUTPUTS =================
#define HORN_PIN 13
#define LIGHT_PIN 23

// ================= DEFAULT TUNING =================
int speedValue = 150;
int trimValue = 0;

bool invertLeft = false;
bool invertRight = false;

int currentLeftSpeed = 0;
int currentRightSpeed = 0;
int targetLeftSpeed = 0;
int targetRightSpeed = 0;

unsigned long lastRampTime = 0;
const int rampStep = 8;
const int rampInterval = 18;

// Draw path calibration
int drawSpeed = 178;
int msPerHalfMeter = 600;
int turnMs90 = 500;

bool hornState = false;
bool lightState = false;
bool hornPulse = false;
bool lightPulse = false;

unsigned long lastHornPulse = 0;
unsigned long lastLightPulse = 0;
bool hornPulseState = false;
bool lightPulseState = false;

bool autoRunning = false;
unsigned long autoStepStart = 0;
int autoStepIndex = 0;

struct AutoStep {
  char cmd;       // F, B, L, R, S, J
  int duration;   // milliseconds
  int x;          // joystick X for J step
  int y;          // joystick Y for J step
  int speed;      // saved speed for J step
};

const int MAX_STEPS = 450;
AutoStep autoSteps[MAX_STEPS];
int autoCount = 0;

// ================= MOTOR FUNCTIONS =================
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

void setMotorTarget(int left, int right) {
  enableDrivers();
  targetLeftSpeed = constrain(left, -255, 255);
  targetRightSpeed = constrain(right, -255, 255);
}

void stopCar() {
  targetLeftSpeed = 0;
  targetRightSpeed = 0;
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

void setJoystickMovementWithSpeed(int x, int y, int savedSpeed) {
  int oldSpeed = speedValue;
  speedValue = constrain(savedSpeed, 0, 255);
  setJoystickMovement(x, y);
  speedValue = oldSpeed;
}

void runSimpleCommand(char cmd) {
  int s = drawSpeed;

  if (cmd == 'F') setMotorTarget(s, s);
  else if (cmd == 'B') setMotorTarget(-s, -s);
  else if (cmd == 'L') setMotorTarget(-s, s);
  else if (cmd == 'R') setMotorTarget(s, -s);
  else stopCar();
}

void startAutoStep(int index) {
  if (index < 0 || index >= autoCount) return;
  if (autoSteps[index].cmd == 'J') {
    setJoystickMovementWithSpeed(autoSteps[index].x, autoSteps[index].y, autoSteps[index].speed);
  } else {
    runSimpleCommand(autoSteps[index].cmd);
  }
}

void updateAutoRunner() {
  if (!autoRunning) return;

  if (autoStepIndex >= autoCount) {
    autoRunning = false;
    stopCar();
    return;
  }

  unsigned long now = millis();

  if (autoStepStart == 0) {
    autoStepStart = now;
    startAutoStep(autoStepIndex);
    return;
  }

  if (now - autoStepStart >= (unsigned long)autoSteps[autoStepIndex].duration) {
    autoStepIndex++;

    if (autoStepIndex >= autoCount) {
      autoRunning = false;
      autoStepStart = 0;
      stopCar();
      return;
    }

    // Important for accurate joystick replay:
    // do NOT insert a stop between small joystick samples.
    // Directly switch to the next saved x/y/speed target.
    autoStepStart = now;
    startAutoStep(autoStepIndex);
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

void updateHornLight() {
  unsigned long now = millis();

  if (hornPulse) {
    if (now - lastHornPulse >= 165) {
      lastHornPulse = now;
      hornPulseState = !hornPulseState;
      digitalWrite(HORN_PIN, hornPulseState ? HIGH : LOW);
    }
  } else {
    digitalWrite(HORN_PIN, hornState ? HIGH : LOW);
  }

  if (lightPulse) {
    if (now - lastLightPulse >= 165) {
      lastLightPulse = now;
      lightPulseState = !lightPulseState;
      digitalWrite(LIGHT_PIN, lightPulseState ? HIGH : LOW);
    }
  } else {
    digitalWrite(LIGHT_PIN, lightState ? HIGH : LOW);
  }
}

// ================= WEB PAGE =================
String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, minimum-scale=1.0, user-scalable=no">
<title>ESP32-6WD V11 Button Path Control</title>
<style>
*{box-sizing:border-box;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent}
html,body{margin:0;padding:0;width:100%;height:100%;overflow:hidden;background:#f8f8f8;color:#111;font-family:Poppins,Arial,sans-serif;overscroll-behavior:contain}
.main{width:100vw;height:100dvh;padding:7px 9px;display:flex;flex-direction:column;gap:6px;align-items:center;background:radial-gradient(circle at 50% -20%,#fff 0,#f4f4f4 42%,#eeeeee 100%)}
.titleBar{width:100%;max-width:430px;display:grid;grid-template-columns:64px 1fr 64px;align-items:start;gap:6px}
.titleMid{text-align:center;padding-top:1px}h1{margin:0;font-size:46px;font-weight:1000;letter-spacing:.2px;text-shadow:0 4px 5px rgba(0,0,0,.28)}h3{margin:2px 0 0;font-size:26px;font-weight:900;color:#444;letter-spacing:.5px}.iconCol{display:flex;flex-direction:column;gap:4px;align-items:center}.iconBtn,.pulseMini{border:0;border-radius:16px;background:linear-gradient(145deg,#fff,#dedede);box-shadow:5px 5px 11px rgba(0,0,0,.20),-4px -4px 10px rgba(255,255,255,.95),inset 2px 2px 4px rgba(255,255,255,.85),inset -2px -2px 5px rgba(0,0,0,.08);display:flex;align-items:center;justify-content:center;transition:.13s transform,.13s box-shadow,.2s background}.iconBtn{width:58px;height:48px}.pulseMini{width:62px;height:32px;font-size:10px;font-weight:1000;color:#111}.iconBtn svg{width:30px;height:30px}.iconBtn:active,.pulseMini:active,.bigBtn:active,.stopBtn:active{transform:translateY(3px) scale(.97);box-shadow:inset 4px 4px 8px rgba(0,0,0,.22),inset -3px -3px 7px rgba(255,255,255,.9)}.on{color:#fff!important;background:linear-gradient(145deg,#111,#555)!important}
.pulseFlash{color:#fff!important;background:linear-gradient(145deg,#00c853,#007a32)!important;box-shadow:0 0 18px rgba(0,200,83,.75),inset 3px 3px 7px rgba(255,255,255,.25),inset -3px -3px 7px rgba(0,0,0,.25)!important}.canvasWrap{width:min(94vw,380px);padding:8px;border-radius:22px;background:linear-gradient(145deg,#fff,#e7e7e7);box-shadow:9px 9px 20px rgba(0,0,0,.22),-8px -8px 18px rgba(255,255,255,.95),inset 2px 2px 5px rgba(255,255,255,.85),inset -2px -2px 5px rgba(0,0,0,.07)}#pathCanvas{width:100%;aspect-ratio:1/1;border-radius:17px;display:block;touch-action:none;background:#fff;box-shadow:inset 4px 4px 10px rgba(0,0,0,.12),inset -4px -4px 10px rgba(255,255,255,.9)}.actionGrid{width:min(96vw,410px);display:grid;grid-template-columns:repeat(3,1fr);gap:9px}.bigBtn{min-height:58px;border:0;border-radius:18px;font-size:15px;font-weight:1000;color:#111;background:linear-gradient(145deg,#fff,#dfdfdf);box-shadow:6px 6px 13px rgba(0,0,0,.21),-5px -5px 12px rgba(255,255,255,.95),inset 2px 2px 4px rgba(255,255,255,.85),inset -2px -2px 5px rgba(0,0,0,.08);transition:.13s transform,.13s box-shadow}.play{color:#fff;background:linear-gradient(145deg,#168dff,#064bbd)}.reverse{color:#fff;background:linear-gradient(145deg,#ffcb8a,#ff8c00)}.clear{background:linear-gradient(145deg,#fff7d1,#e5b700)}.test{background:linear-gradient(145deg,#ffffff,#d7ecff)}.speedStopRow{width:min(94vw,380px);display:grid;grid-template-columns:1fr 78px;gap:8px;align-items:center}.speedBox,.calBox{border-radius:18px;padding:7px 10px;background:linear-gradient(145deg,#fff,#e8e8e8);box-shadow:6px 6px 14px rgba(0,0,0,.18),-5px -5px 13px rgba(255,255,255,.95)}.speedTop{display:flex;justify-content:space-between;font-size:11px;font-weight:1000;margin-bottom:4px}.val{color:#0b73ff}input[type=range]{width:100%;height:7px;appearance:none;border-radius:20px;background:#ddd;box-shadow:inset 2px 2px 4px rgba(0,0,0,.18),inset -2px -2px 4px rgba(255,255,255,.9)}input[type=range]::-webkit-slider-thumb{appearance:none;width:21px;height:21px;border-radius:50%;background:radial-gradient(circle at 30% 25%,#fff,#ddd 35%,#555 76%,#111);box-shadow:0 4px 8px rgba(0,0,0,.35),inset 2px 2px 5px rgba(255,255,255,.55),inset -2px -2px 5px rgba(0,0,0,.35)}.stopBtn{width:76px;height:76px;border:0;border-radius:50%;font-size:13px;font-weight:1000;color:#fff;background:radial-gradient(circle at 32% 22%,#ffb0a9,#ff3b30 42%,#970000 100%);box-shadow:6px 7px 15px rgba(120,0,0,.35),-4px -4px 11px rgba(255,255,255,.9),inset 3px 3px 7px rgba(255,255,255,.25),inset -4px -5px 9px rgba(0,0,0,.25)}.calBox{width:min(94vw,380px);display:grid;grid-template-columns:1fr 1fr;gap:8px}.calItem label{display:block;font-size:9.8px;font-weight:1000;margin-bottom:3px}.note{width:min(94vw,380px);font-size:9.4px;font-weight:800;color:#555;text-align:center;line-height:1.2;margin-top:-2px}
@media (orientation:landscape){
html,body{overflow:hidden}
.main{
  height:100dvh;
  padding:6px 8px;
  display:grid;
  grid-template-columns:50vw 50vw;
  grid-template-rows:auto auto auto auto 1fr;
  column-gap:8px;
  row-gap:5px;
  align-items:start;
}
.titleBar{
  grid-column:2;
  grid-row:1;
  width:100%;
  max-width:none;
  grid-template-columns:62px 1fr 62px;
  align-items:start;
  padding-top:0;
}
.titleMid{padding-top:0}
h1{font-size:26px;line-height:.95}
h3{font-size:15px;margin-top:1px}
.iconCol{gap:3px}
.iconBtn{width:54px;height:36px;border-radius:14px}
.iconBtn svg{width:24px;height:24px}
.pulseMini{width:58px;height:24px;border-radius:12px;font-size:8.5px}
.canvasWrap{
  grid-column:1;
  grid-row:1 / 6;
  width:calc(50vw - 14px);
  height:calc(100dvh - 12px);
  max-width:none;
  padding:6px;
  align-self:start;
}
#pathCanvas{
  width:100%;
  height:100%;
  aspect-ratio:auto;
}
.actionGrid{
  grid-column:2;
  grid-row:2;
  width:calc(50vw - 18px);
  display:grid;
  grid-template-columns:repeat(3,1fr);
  gap:7px;
  align-self:start;
}
.bigBtn{min-height:43px;font-size:10px;border-radius:13px;line-height:1.05}
.speedStopRow{
  grid-column:2;
  grid-row:3;
  width:calc(50vw - 18px);
  grid-template-columns:1fr 58px;
  gap:6px;
}
.stopBtn{width:56px;height:56px;font-size:10px}
.speedBox,.calBox{padding:5px 8px;border-radius:14px}
.speedTop{font-size:9.5px;margin-bottom:2px}
.calBox{
  grid-column:2;
  grid-row:4;
  width:calc(50vw - 18px);
  grid-template-columns:1fr 1fr;
  gap:6px;
}
.calItem label{font-size:8px;margin-bottom:1px}
input[type=range]{height:5px}
input[type=range]::-webkit-slider-thumb{width:17px;height:17px}
.note{
  grid-column:2;
  grid-row:5;
  width:calc(50vw - 18px);
  font-size:8.4px;
  line-height:1.15;
  margin-top:0;
  display:block;
}
}
@media (max-height:700px){.main{gap:4px;padding:5px 8px}h1{font-size:30px}h3{font-size:20px}.canvasWrap{width:min(88vw,335px);padding:6px}.bigBtn{min-height:40px;font-size:9.5px}.stopBtn{width:66px;height:66px}.iconBtn{height:42px}.pulseMini{height:26px}.speedBox,.calBox{padding:6px 9px}.note{display:none}}
</style>
</head>
<body>
<div class="main">
  <div class="titleBar">
    <div class="iconCol">
      <button id="hornBtn" class="iconBtn" title="Horn" onpointerdown="hornDown()" onpointerup="hornUp()" onpointercancel="hornUp()"><svg viewBox="0 0 64 64"><path fill="currentColor" d="M9 26h10l15-12v36L19 38H9z"></path><path fill="none" stroke="currentColor" stroke-width="6" stroke-linecap="round" d="M44 22c5 5 5 15 0 20M52 14c10 10 10 26 0 36"></path></svg></button>
      <button id="pulseHornBtn" class="pulseMini" onclick="togglePulseHorn()">PULSE</button>
    </div>
    <div class="titleMid"><h1>ESP32 - 6WD</h1><h3>Draw Path Control - V11</h3></div>
    <div class="iconCol">
      <button id="lightBtn" class="iconBtn" title="Light" onclick="toggleLight()"><svg viewBox="0 0 64 64"><path fill="currentColor" d="M8 14c18 0 32 8 32 18S26 50 8 50c-4 0-6-2.5-6-6V20c0-3.5 2-6 6-6z"></path><path fill="none" stroke="currentColor" stroke-width="5" stroke-linecap="round" d="M48 18h10M48 32h12M48 46h10"></path></svg></button>
      <button id="pulseLightBtn" class="pulseMini" onclick="togglePulseLight()">PULSE</button>
    </div>
  </div>

  <div class="canvasWrap"><canvas id="pathCanvas" width="720" height="720"></canvas></div>

  <div class="actionGrid">
    <button class="bigBtn play" onclick="playDrawPath(false)">PLAY<br>PATH</button>
    <button class="bigBtn reverse" onclick="playDrawPath(true)">REVERSE<br>PATH</button>
    <button class="bigBtn clear" onclick="clearCanvas()">CLEAR<br>PATH</button>
    <button class="bigBtn test" onclick="testTurn()">TURN<br>90°</button>
    <button class="bigBtn test" onclick="testForward()">TEST FWD<br>0.5m</button>
    <button class="bigBtn test" onclick="testBackward()">TEST BACK<br>0.5m</button>
  </div>

  <div class="speedStopRow">
    <div class="speedBox">
      <div class="speedTop"><span>Car Speed</span><span id="speedVal" class="val">70%</span></div>
      <input id="carSpeedSlider" type="range" min="25" max="255" value="178" oninput="setCarSpeed(this.value)">
    </div>
    <button class="stopBtn" onclick="emergencyStop()">STOP</button>
  </div>

  <div class="calBox">
    <div class="calItem"><label>0.5m time @70%</label><input id="halfMeterSlider" type="range" min="250" max="2200" value="600" oninput="setHalfMeter(this.value)"><label id="halfMeterVal" class="val">600ms</label></div>
    <div class="calItem"><label>90° turn @70%</label><input id="turnSlider" type="range" min="220" max="1800" value="500" oninput="setTurnMs(this.value)"><label id="turnVal" class="val">500ms</label></div>
  </div>

  <div class="note">First touch = car start. Release = end. Drawing speed ignore hoti hai; Car Speed slider fixed motor speed hai. 0.5m/90° sliders calibration values default 70% speed par set hain.</div>
</div>
<script>
document.addEventListener('contextmenu',e=>e.preventDefault());
document.addEventListener('selectstart',e=>e.preventDefault());
document.addEventListener('copy',e=>e.preventDefault());
document.addEventListener('dragstart',e=>e.preventDefault());document.addEventListener('gesturestart',e=>e.preventDefault());document.addEventListener('dblclick',e=>e.preventDefault());
function api(u){fetch(u).catch(()=>{});}
function colorByPercent(p){let h=210-(p*2.1);return `hsl(${h},100%,45%)`;}
function paintSlider(el,p){let c=colorByPercent(p);el.style.background=`linear-gradient(90deg,${c} 0%,${c} ${p}%,#dedede ${p}%,#dedede 100%)`;}
function carSpeed(){return Number(document.getElementById('carSpeedSlider').value)}
function carPercent(){return Math.round(carSpeed()/255*100)}
function setCarSpeed(v){let p=Math.round(v/255*100);document.getElementById('speedVal').innerHTML=p+'%';paintSlider(document.getElementById('carSpeedSlider'),p);api('/drawSpeed?value='+v)}
function setHalfMeter(v){document.getElementById('halfMeterVal').innerHTML=v+'ms';api('/cal?half='+v)}
function setTurnMs(v){document.getElementById('turnVal').innerHTML=v+'ms';api('/cal?turn='+v)}
let hornP=false,light=false,lightP=false,pulseUiFlip=false;
setInterval(()=>{
  pulseUiFlip=!pulseUiFlip;
  document.getElementById('pulseHornBtn').classList.toggle('pulseFlash',hornP && pulseUiFlip);
  document.getElementById('pulseLightBtn').classList.toggle('pulseFlash',lightP && pulseUiFlip);
},165); // same 3-times-per-second pulse timing as ESP32 output
function hornDown(){
  hornP=false;
  document.getElementById('pulseHornBtn').classList.remove('on','pulseFlash');
  document.getElementById('hornBtn').classList.add('on');
  api('/pulseHorn?state=0');api('/horn?state=1');
}
function hornUp(){document.getElementById('hornBtn').classList.remove('on');api('/horn?state=0')}
function togglePulseHorn(){
  hornP=!hornP;
  document.getElementById('pulseHornBtn').classList.toggle('on',hornP);
  document.getElementById('pulseHornBtn').classList.remove('pulseFlash');
  api('/pulseHorn?state='+(hornP?1:0));
}
function toggleLight(){
  light=!light;
  if(light){lightP=false;document.getElementById('pulseLightBtn').classList.remove('on','pulseFlash');api('/pulseLight?state=0');}
  document.getElementById('lightBtn').classList.toggle('on',light);
  api('/light?state='+(light?1:0));
}
function togglePulseLight(){
  lightP=!lightP;
  if(lightP){light=false;document.getElementById('lightBtn').classList.remove('on');api('/light?state=0');}
  document.getElementById('pulseLightBtn').classList.toggle('on',lightP);
  document.getElementById('pulseLightBtn').classList.remove('pulseFlash');
  api('/pulseLight?state='+(lightP?1:0));
}
function emergencyStop(){
  hornP=false; light=false; lightP=false;
  document.getElementById('pulseHornBtn').classList.remove('on');
  document.getElementById('lightBtn').classList.remove('on');
  document.getElementById('pulseLightBtn').classList.remove('on');
  api('/stop')
}
const canvas=document.getElementById('pathCanvas'),ctx=canvas.getContext('2d');
let drawing=false,points=[];
function cpos(e){const r=canvas.getBoundingClientRect();return{x:(e.clientX-r.left)*canvas.width/r.width,y:(e.clientY-r.top)*canvas.height/r.height}}
function drawGrid(){const w=canvas.width,h=canvas.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#fbfbfb';ctx.fillRect(0,0,w,h);let step=w/8;ctx.lineWidth=1;for(let i=0;i<=8;i++){let p=i*step;ctx.strokeStyle=i%2===0?'#b8b8b8':'#dddddd';ctx.beginPath();ctx.moveTo(p,0);ctx.lineTo(p,h);ctx.stroke();ctx.beginPath();ctx.moveTo(0,p);ctx.lineTo(w,p);ctx.stroke();if(i>0){ctx.fillStyle='#333';ctx.font='bold 25px Arial';ctx.fillText((i*.5).toFixed(1)+'m',p+8,32);ctx.fillText((i*.5).toFixed(1)+'m',10,p-8)}}ctx.strokeStyle='#111';ctx.lineWidth=4;ctx.strokeRect(2,2,w-4,h-4);if(points.length){let s=points[0];ctx.fillStyle='#111';ctx.beginPath();ctx.arc(s.x,s.y,13,0,Math.PI*2);ctx.fill();ctx.font='bold 24px Arial';ctx.fillText('START',Math.min(s.x+18,w-95),Math.max(s.y-14,28))}if(points.length>1){ctx.strokeStyle='#0b8cff';ctx.lineWidth=12;ctx.lineCap='round';ctx.lineJoin='round';ctx.beginPath();ctx.moveTo(points[0].x,points[0].y);for(const p of points)ctx.lineTo(p.x,p.y);ctx.stroke();let e=points[points.length-1];ctx.fillStyle='#ff3b30';ctx.beginPath();ctx.arc(e.x,e.y,15,0,Math.PI*2);ctx.fill();ctx.font='bold 24px Arial';ctx.fillText('END',Math.min(e.x+18,w-70),Math.min(e.y+30,h-15))}}
canvas.addEventListener('pointerdown',e=>{drawing=true;canvas.setPointerCapture(e.pointerId);points=[cpos(e)];drawGrid()});
canvas.addEventListener('pointermove',e=>{if(!drawing)return;let p=cpos(e),l=points[points.length-1];if(Math.hypot(p.x-l.x,p.y-l.y)>9){points.push(p);drawGrid()}});
function endDraw(){drawing=false;drawGrid()}canvas.addEventListener('pointerup',endDraw);canvas.addEventListener('pointercancel',endDraw);
function clearCanvas(){points=[];drawGrid();stopPreview();api('/moveStop')}
let previewTimer=null, previewRunning=false, previewIndex=0, previewSteps=[], carPose=null;
function stopPreview(){previewRunning=false;if(previewTimer){clearTimeout(previewTimer);previewTimer=null}}
function resetCarIcon(){stopPreview();carPose=null;drawGrid()}
function drawCarIcon(x,y,ang){
  ctx.save();
  ctx.translate(x,y);
  ctx.fillStyle='#0b8cff';
  ctx.beginPath();
  ctx.arc(0,0,16,0,Math.PI*2);
  ctx.fill();
  ctx.strokeStyle='#ffffff';
  ctx.lineWidth=3;
  ctx.beginPath();
  ctx.moveTo(0,0);
  ctx.lineTo(Math.cos(ang)*30,Math.sin(ang)*30);
  ctx.stroke();
  ctx.restore();
}
function drawGrid(){
  const w=canvas.width,h=canvas.height;
  ctx.clearRect(0,0,w,h);
  ctx.fillStyle='#fbfbfb';ctx.fillRect(0,0,w,h);
  let step=w/8;
  ctx.lineWidth=1;
  for(let i=0;i<=8;i++){
    let p=i*step;
    ctx.strokeStyle=i%2===0?'#b8b8b8':'#dddddd';
    ctx.beginPath();ctx.moveTo(p,0);ctx.lineTo(p,h);ctx.stroke();
    ctx.beginPath();ctx.moveTo(0,p);ctx.lineTo(w,p);ctx.stroke();
    if(i>0){
      ctx.fillStyle='#333';ctx.font='bold 25px Arial';
      ctx.fillText((i*.5).toFixed(1)+'m',p+8,32);
      ctx.fillText((i*.5).toFixed(1)+'m',10,p-8);
    }
  }
  ctx.strokeStyle='#111';ctx.lineWidth=4;ctx.strokeRect(2,2,w-4,h-4);
  if(points.length>1){
    ctx.strokeStyle='#0b8cff';ctx.lineWidth=12;ctx.lineCap='round';ctx.lineJoin='round';
    ctx.beginPath();ctx.moveTo(points[0].x,points[0].y);
    for(const p of points)ctx.lineTo(p.x,p.y);
    ctx.stroke();
  }
  if(points.length){
    let st=points[0];
    ctx.fillStyle='#111';ctx.beginPath();ctx.arc(st.x,st.y,13,0,Math.PI*2);ctx.fill();
    ctx.font='bold 24px Arial';ctx.fillText('START',Math.min(st.x+18,w-95),Math.max(st.y-14,28));
  }
  if(points.length>1){
    let en=points[points.length-1];
    ctx.fillStyle='#ff3b30';ctx.beginPath();ctx.arc(en.x,en.y,15,0,Math.PI*2);ctx.fill();
    ctx.font='bold 24px Arial';ctx.fillText('END',Math.min(en.x+18,w-70),Math.min(en.y+30,h-15));
  }
  if(carPose)drawCarIcon(carPose.x,carPose.y,carPose.a);
}
const REF_SPEED=178; // tested draw-path speed baseline
function clamp(v,a,b){return Math.max(a,Math.min(b,v))}
function normAng(a){while(a>Math.PI)a-=Math.PI*2;while(a<-Math.PI)a+=Math.PI*2;return a}
function metricDuration(px){
  const gridPx=canvas.width/8;       // 1 grid = 0.5 meter
  const spd=Math.max(25,carSpeed());
  const speedFactor=REF_SPEED/spd;
  const halfMs=Number(document.getElementById('halfMeterSlider').value);
  return clamp(Math.round((px/gridPx)*halfMs*speedFactor),80,9000);
}
function turnDuration(){
  const spd=Math.max(25,carSpeed());
  return clamp(Math.round(Number(document.getElementById('turnSlider').value)*(REF_SPEED/spd)),80,2500);
}
function pathLength(pts){let l=0;for(let i=1;i<pts.length;i++)l+=Math.hypot(pts[i].x-pts[i-1].x,pts[i].y-pts[i-1].y);return l}
function rdp(pts,eps){
  if(pts.length<3)return pts.slice();
  const a=pts[0],b=pts[pts.length-1];let maxD=0,idx=0;
  const dx=b.x-a.x,dy=b.y-a.y,den=Math.hypot(dx,dy)||1;
  for(let i=1;i<pts.length-1;i++){
    const p=pts[i];
    const d=Math.abs(dy*p.x-dx*p.y+b.x*a.y-b.y*a.x)/den;
    if(d>maxD){maxD=d;idx=i}
  }
  if(maxD>eps){
    const left=rdp(pts.slice(0,idx+1),eps), right=rdp(pts.slice(idx),eps);
    return left.slice(0,-1).concat(right);
  }
  return [a,b];
}
function isCircleLike(pts){
  if(pts.length<18)return false;
  const len=pathLength(pts); if(len<canvas.width*0.45)return false;
  const first=pts[0],last=pts[pts.length-1];
  const close=Math.hypot(first.x-last.x,first.y-last.y)<Math.max(70,canvas.width*0.16);
  let turn=0,prev=null;
  for(let i=1;i<pts.length;i++){
    const a=Math.atan2(pts[i].y-pts[i-1].y,pts[i].x-pts[i-1].x);
    if(prev!==null)turn+=normAng(a-prev);
    prev=a;
  }
  return close && Math.abs(turn)>Math.PI*1.25;
}
function circlePrimitive(){
  const len=pathLength(points);
  let area=0;
  for(let i=1;i<points.length;i++) area+=(points[i].x-points[i-1].x)*(points[i].y+points[i-1].y);
  // canvas coordinates: area positive roughly means clockwise path
  const clockwise=area>0;
  const w=canvas.width,h=canvas.height;
  let minX=w,maxX=0,minY=h,maxY=0;
  for(const p of points){minX=Math.min(minX,p.x);maxX=Math.max(maxX,p.x);minY=Math.min(minY,p.y);maxY=Math.max(maxY,p.y)}
  const diameter=Math.max(maxX-minX,maxY-minY,1);
  // Big circle = small steering difference. Small circle = bigger steering difference.
  let steer=clamp(Math.round((canvas.width*0.32/diameter)*45),18,70);
  if(!clockwise)steer=-steer;
  const dur=metricDuration(len);
  return [{cmd:'J',x:steer,y:82,speed:carSpeed(),duration:dur,sx:points[0].x,sy:points[0].y,ex:points[points.length-1].x,ey:points[points.length-1].y,angle:0},{cmd:'S',duration:120}];
}
function segmentPrimitive(a,b,currentHeading){
  const dx=b.x-a.x,dy=b.y-a.y,dist=Math.hypot(dx,dy);
  if(dist<18)return {steps:[],heading:currentHeading};
  const target=Math.atan2(dy,dx);
  let delta=normAng(target-currentHeading);
  // Tiny hand tilt ignore: treat as straight.
  if(Math.abs(delta)<0.35){
    return {steps:[{cmd:'F',duration:metricDuration(dist),sx:a.x,sy:a.y,ex:b.x,ey:b.y,angle:currentHeading}],heading:currentHeading};
  }
  // Reverse line: use back button if segment is opposite of current heading.
  if(Math.abs(Math.abs(delta)-Math.PI)<0.40){
    return {steps:[{cmd:'B',duration:metricDuration(dist),sx:a.x,sy:a.y,ex:b.x,ey:b.y,angle:currentHeading}],heading:currentHeading};
  }
  let steps=[];
  const ninety=Math.PI/2;
  let quarter=Math.round(delta/ninety);
  quarter=clamp(quarter,-2,2);
  if(quarter===2 || quarter===-2){
    steps.push({cmd:'R',duration:turnDuration(),sx:a.x,sy:a.y,ex:a.x,ey:a.y,angle:currentHeading+ninety});
    steps.push({cmd:'R',duration:turnDuration(),sx:a.x,sy:a.y,ex:a.x,ey:a.y,angle:currentHeading+Math.PI});
    currentHeading=normAng(currentHeading+Math.PI);
  }else if(quarter===1){
    steps.push({cmd:'R',duration:turnDuration(),sx:a.x,sy:a.y,ex:a.x,ey:a.y,angle:currentHeading+ninety});
    currentHeading=normAng(currentHeading+ninety);
  }else if(quarter===-1){
    steps.push({cmd:'L',duration:turnDuration(),sx:a.x,sy:a.y,ex:a.x,ey:a.y,angle:currentHeading-ninety});
    currentHeading=normAng(currentHeading-ninety);
  }
  steps.push({cmd:'F',duration:metricDuration(dist),sx:a.x,sy:a.y,ex:b.x,ey:b.y,angle:currentHeading});
  return {steps,heading:currentHeading};
}
function forwardPrimitives(){
  if(points.length<2)return [];
  if(isCircleLike(points))return circlePrimitive();
  const eps=canvas.width*0.055; // ignore hand wobble; keep sharp corners
  let raw=rdp(points,eps);
  // Remove tiny leftover pieces.
  let simple=[];
  for(const p of raw){
    if(!simple.length || Math.hypot(p.x-simple[simple.length-1].x,p.y-simple[simple.length-1].y)>canvas.width*0.035) simple.push(p);
  }
  if(simple.length<2)return [];
  let prim=[];
  let heading=-Math.PI/2; // car starts facing upward on graph
  for(let i=1;i<simple.length;i++){
    const r=segmentPrimitive(simple[i-1],simple[i],heading);
    prim=prim.concat(r.steps);
    heading=r.heading;
  }
  prim.push({cmd:'S',duration:120});
  return prim;
}
function reversePrimitives(fwd){
  let rev=[];
  for(let i=fwd.length-1;i>=0;i--){
    let p=fwd[i]; if(p.cmd==='S')continue;
    if(p.cmd==='F')rev.push({...p,cmd:'B',sx:p.ex,sy:p.ey,ex:p.sx,ey:p.sy});
    else if(p.cmd==='B')rev.push({...p,cmd:'F',sx:p.ex,sy:p.ey,ex:p.sx,ey:p.sy});
    else if(p.cmd==='R')rev.push({...p,cmd:'L'});
    else if(p.cmd==='L')rev.push({...p,cmd:'R'});
    else if(p.cmd==='J')rev.push({...p,x:-p.x,y:-p.y,sx:p.ex,sy:p.ey,ex:p.sx,ey:p.sy});
  }
  rev.push({cmd:'S',duration:120});
  return rev;
}
function primitivesToApi(prim){
  return prim.map(p=>{
    if(p.cmd==='J')return `J:${p.x}:${p.y}:${p.speed}:${p.duration}`;
    return `${p.cmd}:${p.duration}`;
  }).join(',');
}
function runPreview(prim){
  stopPreview();
  if(!prim.length)return;
  previewRunning=true;previewSteps=prim;previewIndex=0;
  let px=points[0]?.x||canvas.width/2, py=points[0]?.y||canvas.height/2, ang=-Math.PI/2;
  function step(){
    if(!previewRunning || previewIndex>=previewSteps.length){stopPreview();drawGrid();return}
    let sg=previewSteps[previewIndex++];
    if(sg.cmd==='S'){previewTimer=setTimeout(step,Math.min(160,sg.duration));return}
    let startT=performance.now(),sx=px,sy=py,sa=ang;
    function frame(t){
      if(!previewRunning)return;
      let k=Math.min(1,(t-startT)/sg.duration);
      if(sg.cmd==='R'||sg.cmd==='L'){
        let dir=sg.cmd==='R'?1:-1;
        ang=sa+dir*(Math.PI/2)*k; carPose={x:sx,y:sy,a:ang};
      }else if(sg.cmd==='F'||sg.cmd==='B'||sg.cmd==='J'){
        px=sg.sx+(sg.ex-sg.sx)*k; py=sg.sy+(sg.ey-sg.sy)*k; ang=sg.angle||ang; carPose={x:px,y:py,a:ang};
      }
      drawGrid();
      if(k<1)requestAnimationFrame(frame); else{previewTimer=setTimeout(step,2)}
    }
    requestAnimationFrame(frame);
  }
  step();
}
function playDrawPath(reverse=false){
  let fwd=forwardPrimitives();
  let prim=reverse?reversePrimitives(fwd):fwd;
  if(prim.length<2)return;
  api('/runPath?steps='+encodeURIComponent(primitivesToApi(prim)));
  runPreview(prim);
}
function testForward(){let spd=carSpeed();let dur=Math.round(Number(document.getElementById('halfMeterSlider').value)*(REF_SPEED/spd));api('/runPath?steps='+encodeURIComponent(`F:${dur},S:80`))}
function testBackward(){let spd=carSpeed();let dur=Math.round(Number(document.getElementById('halfMeterSlider').value)*(REF_SPEED/spd));api('/runPath?steps='+encodeURIComponent(`B:${dur},S:80`))}
function testTurn(){let dur=turnDuration();api('/runPath?steps='+encodeURIComponent('R:'+dur+',S:80'))}
setCarSpeed(178);setHalfMeter(600);setTurnMs(500);drawGrid();
</script>
</body>
</html>
)rawliteral";

// ================= HANDLERS =================
void handleRoot() {
  server.send(200, "text/html", page);
}

void handleJoy() {
  autoRunning = false;

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

void handleCal() {
  if (server.hasArg("half")) {
    msPerHalfMeter = server.arg("half").toInt();
    msPerHalfMeter = constrain(msPerHalfMeter, 300, 2200);
  }

  if (server.hasArg("turn")) {
    turnMs90 = server.arg("turn").toInt();
    turnMs90 = constrain(turnMs90, 250, 1800);
  }

  server.send(200, "text/plain", "OK");
}

void handleDrawSpeed() {
  if (server.hasArg("value")) {
    drawSpeed = server.arg("value").toInt();
    drawSpeed = constrain(drawSpeed, 0, 255);
  }

  server.send(200, "text/plain", "OK");
}

void handleHorn() {
  if (server.hasArg("state")) {
    hornState = server.arg("state").toInt() == 1;
    if (hornState) hornPulse = false;
  }
  server.send(200, "text/plain", "OK");
}

void handlePulseHorn() {
  if (server.hasArg("state")) {
    hornPulse = server.arg("state").toInt() == 1;
    if (!hornPulse) {
      hornPulseState = false;
      digitalWrite(HORN_PIN, hornState ? HIGH : LOW);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleLight() {
  if (server.hasArg("state")) {
    lightState = server.arg("state").toInt() == 1;
    if (lightState) lightPulse = false;
  }
  server.send(200, "text/plain", "OK");
}

void handlePulseLight() {
  if (server.hasArg("state")) {
    lightPulse = server.arg("state").toInt() == 1;
    if (!lightPulse) {
      lightPulseState = false;
      digitalWrite(LIGHT_PIN, lightState ? HIGH : LOW);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleMoveStop() {
  autoRunning = false;
  autoStepIndex = 0;
  autoStepStart = 0;
  stopCar();
  server.send(200, "text/plain", "MOVE STOPPED");
}

void handleStop() {
  autoRunning = false;
  autoStepIndex = 0;
  autoStepStart = 0;
  stopCar();

  hornState = false;
  lightState = false;
  hornPulse = false;
  lightPulse = false;
  hornPulseState = false;
  lightPulseState = false;
  digitalWrite(HORN_PIN, LOW);
  digitalWrite(LIGHT_PIN, LOW);

  server.send(200, "text/plain", "ALL STOPPED");
}

void addAutoStep(char cmd, int duration) {
  if (autoCount >= MAX_STEPS) return;
  if (!(cmd == 'F' || cmd == 'B' || cmd == 'L' || cmd == 'R' || cmd == 'S')) return;
  duration = constrain(duration, 50, 8000);
  autoSteps[autoCount].cmd = cmd;
  autoSteps[autoCount].duration = duration;
  autoSteps[autoCount].x = 0;
  autoSteps[autoCount].y = 0;
  autoSteps[autoCount].speed = drawSpeed;
  autoCount++;
}

void addJoystickAutoStep(int x, int y, int spd, int duration) {
  if (autoCount >= MAX_STEPS) return;
  duration = constrain(duration, 40, 8000);
  autoSteps[autoCount].cmd = 'J';
  autoSteps[autoCount].duration = duration;
  autoSteps[autoCount].x = constrain(x, -100, 100);
  autoSteps[autoCount].y = constrain(y, -100, 100);
  autoSteps[autoCount].speed = constrain(spd, 0, 255);
  autoCount++;
}

void handleRunPath() {
  autoRunning = false;
  stopCar();

  autoCount = 0;
  autoStepIndex = 0;
  autoStepStart = 0;

  if (!server.hasArg("steps")) {
    server.send(400, "text/plain", "NO STEPS");
    return;
  }

  String steps = server.arg("steps");
  steps += ",";

  int start = 0;
  while (true) {
    int comma = steps.indexOf(',', start);
    if (comma < 0) break;

    String token = steps.substring(start, comma);
    token.trim();

    int colon = token.indexOf(':');
    if (colon > 0) {
      char cmd = token.charAt(0);
      if (cmd == 'J') {
        int c1 = token.indexOf(':');
        int c2 = token.indexOf(':', c1 + 1);
        int c3 = token.indexOf(':', c2 + 1);
        int c4 = token.indexOf(':', c3 + 1);
        if (c1 > 0 && c2 > c1 && c3 > c2 && c4 > c3) {
          int x = token.substring(c1 + 1, c2).toInt();
          int y = token.substring(c2 + 1, c3).toInt();
          int spd = token.substring(c3 + 1, c4).toInt();
          int duration = token.substring(c4 + 1).toInt();
          addJoystickAutoStep(x, y, spd, duration);
        }
      } else {
        int duration = token.substring(colon + 1).toInt();
        addAutoStep(cmd, duration);
      }
    }

    start = comma + 1;
    if (start >= steps.length()) break;
  }

  if (autoCount > 0) {
    autoRunning = true;
    autoStepIndex = 0;
    autoStepStart = 0;
  }

  server.send(200, "text/plain", "RUNNING");
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_L_EN, OUTPUT);
  pinMode(LEFT_R_EN, OUTPUT);
  pinMode(RIGHT_L_EN, OUTPUT);
  pinMode(RIGHT_R_EN, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(HORN_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);

  digitalWrite(HORN_PIN, LOW);
  digitalWrite(LIGHT_PIN, LOW);
  digitalWrite(BLUE_LED, LOW);

  setupPWM();
  enableDrivers();
  stopCar();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  esp_wifi_set_max_tx_power(78);

  server.on("/", handleRoot);
  server.on("/joy", handleJoy);
  server.on("/speed", handleSpeed);
  server.on("/trim", handleTrim);
  server.on("/cal", handleCal);
  server.on("/drawSpeed", handleDrawSpeed);
  server.on("/horn", handleHorn);
  server.on("/pulseHorn", handlePulseHorn);
  server.on("/light", handleLight);
  server.on("/pulseLight", handlePulseLight);
  server.on("/stop", handleStop);
  server.on("/moveStop", handleMoveStop);
  server.on("/runPath", handleRunPath);

  server.begin();

  // One short beep after AP start, instead of accidental car movement.
  digitalWrite(HORN_PIN, HIGH);
  delay(120);
  digitalWrite(HORN_PIN, LOW);
}

void loop() {
  server.handleClient();
  updateAutoRunner();
  updateMotorsSmooth();
  updateHornLight();
}
