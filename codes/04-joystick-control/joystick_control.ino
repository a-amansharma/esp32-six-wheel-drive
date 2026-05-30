#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "SixWheelCar";
const char* password = "12345678";

WebServer server(80);

#define L_RPWM 27
#define L_LPWM 14
#define L_REN 25
#define L_LEN 26

#define R_RPWM 18
#define R_LPWM 19
#define R_REN 32
#define R_LEN 33

int speedLimit = 140;

int currentLeft = 0;
int currentRight = 0;
int targetLeft = 0;
int targetRight = 0;

bool forwardLatch = false;
bool backwardLatch = false;
bool leftPressed = false;
bool rightPressed = false;

bool joystickActive = false;

unsigned long lastRampTime = 0;
const int rampStep = 6;
const int rampDelay = 10;

void leftMotor(int spd) {
  spd = constrain(spd, -255, 255);

  if (spd > 0) {
    ledcWrite(L_RPWM, spd);
    ledcWrite(L_LPWM, 0);
  } else if (spd < 0) {
    ledcWrite(L_RPWM, 0);
    ledcWrite(L_LPWM, -spd);
  } else {
    ledcWrite(L_RPWM, 0);
    ledcWrite(L_LPWM, 0);
  }
}

void rightMotor(int spd) {
  spd = constrain(spd, -255, 255);

  if (spd > 0) {
    ledcWrite(R_RPWM, spd);
    ledcWrite(R_LPWM, 0);
  } else if (spd < 0) {
    ledcWrite(R_RPWM, 0);
    ledcWrite(R_LPWM, -spd);
  } else {
    ledcWrite(R_RPWM, 0);
    ledcWrite(R_LPWM, 0);
  }
}

void updateMotors() {
  if (millis() - lastRampTime < rampDelay) return;

  lastRampTime = millis();

  if (currentLeft < targetLeft) {
    currentLeft += rampStep;
    if (currentLeft > targetLeft) currentLeft = targetLeft;
  } else if (currentLeft > targetLeft) {
    currentLeft -= rampStep;
    if (currentLeft < targetLeft) currentLeft = targetLeft;
  }

  if (currentRight < targetRight) {
    currentRight += rampStep;
    if (currentRight > targetRight) currentRight = targetRight;
  } else if (currentRight > targetRight) {
    currentRight -= rampStep;
    if (currentRight < targetRight) currentRight = targetRight;
  }

  leftMotor(currentLeft);
  rightMotor(currentRight);
}

void setTargets(int l, int r) {
  targetLeft = constrain(l, -255, 255);
  targetRight = constrain(r, -255, 255);
}

void updateButtonDrive() {
  int throttle = 0;
  int steering = 0;

  if (forwardLatch) throttle += speedLimit;
  if (backwardLatch) throttle -= speedLimit;

  if (leftPressed) steering += speedLimit;
  if (rightPressed) steering -= speedLimit;

  int leftOut = -(throttle + steering);
  int rightOut = -(throttle - steering);

  leftOut = constrain(leftOut, -speedLimit, speedLimit);
  rightOut = constrain(rightOut, -speedLimit, speedLimit);

  setTargets(leftOut, rightOut);
}

void pressUp() {
  joystickActive = false;
  forwardLatch = true;
  backwardLatch = false;
  updateButtonDrive();
  server.send(200, "text/plain", "UP");
}

void releaseUp() {
  updateButtonDrive();
  server.send(200, "text/plain", "UP_RELEASE");
}

void pressDown() {
  joystickActive = false;
  backwardLatch = true;
  forwardLatch = false;
  updateButtonDrive();
  server.send(200, "text/plain", "DOWN");
}

void releaseDown() {
  updateButtonDrive();
  server.send(200, "text/plain", "DOWN_RELEASE");
}

void pressLeft() {
  joystickActive = false;
  leftPressed = true;
  rightPressed = false;
  updateButtonDrive();
  server.send(200, "text/plain", "LEFT");
}

void releaseLeft() {
  leftPressed = false;
  updateButtonDrive();
  server.send(200, "text/plain", "LEFT_RELEASE");
}

void pressRight() {
  joystickActive = false;
  rightPressed = true;
  leftPressed = false;
  updateButtonDrive();
  server.send(200, "text/plain", "RIGHT");
}

void releaseRight() {
  rightPressed = false;
  updateButtonDrive();
  server.send(200, "text/plain", "RIGHT_RELEASE");
}

void stopWeb() {
  joystickActive = false;
  forwardLatch = false;
  backwardLatch = false;
  leftPressed = false;
  rightPressed = false;
  setTargets(0, 0);
  server.send(200, "text/plain", "STOP");
}

void setSpeed() {
  if (server.hasArg("value")) {
    speedLimit = server.arg("value").toInt();
    speedLimit = constrain(speedLimit, 0, 255);

    if (!joystickActive) {
      updateButtonDrive();
    }
  }

  server.send(200, "text/plain", String(speedLimit));
}

void joystick() {
  if (server.hasArg("x") && server.hasArg("y")) {
    joystickActive = true;

    forwardLatch = false;
    backwardLatch = false;
    leftPressed = false;
    rightPressed = false;

    int x = server.arg("x").toInt();
    int y = server.arg("y").toInt();

    x = constrain(x, -100, 100);
    y = constrain(y, -100, 100);

    int throttle = map(y, -100, 100, -speedLimit, speedLimit);
    int steering = map(x, -100, 100, speedLimit, -speedLimit);

    int leftOut = -(throttle + steering);
    int rightOut = -(throttle - steering);

    leftOut = constrain(leftOut, -speedLimit, speedLimit);
    rightOut = constrain(rightOut, -speedLimit, speedLimit);

    setTargets(leftOut, rightOut);
  }

  server.send(200, "text/plain", "JOY");
}

void joystickStop() {
  joystickActive = false;
  setTargets(0, 0);
  server.send(200, "text/plain", "JOY_STOP");
}

void handleRoot() {
  String page =
  "<html><head>"
  "<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no'>"
  "<style>"
  "*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;touch-action:none;user-select:none;}"
  "html,body{margin:0;padding:0;width:100%;height:100%;overflow:hidden;background:#fff;font-family:Arial,Helvetica,sans-serif;color:#111827;}"
  ".app{width:100vw;height:100vh;display:flex;flex-direction:column;align-items:center;padding:8px 10px;}"
  ".title{font-size:22px;font-weight:900;margin:3px 0;color:#111827;letter-spacing:.5px;}"
  ".sub{font-size:12px;color:#6b7280;margin-bottom:4px;}"
  ".top{width:100%;max-width:370px;background:#f8fafc;border:1px solid #e5e7eb;border-radius:22px;padding:8px;box-shadow:0 5px 14px rgba(0,0,0,.10);}"
  ".control{display:grid;grid-template-columns:72px 72px 72px;grid-template-rows:56px 56px 56px;gap:7px;justify-content:center;align-items:center;}"
  ".btn{border:0;border-radius:18px;background:linear-gradient(145deg,#2563eb,#1d4ed8);color:white;font-weight:900;font-size:14px;box-shadow:0 5px 12px rgba(37,99,235,.30);}"
  ".btn:active{transform:scale(.94);}"
  ".up{grid-column:2;grid-row:1;width:72px;height:56px;}"
  ".left{grid-column:1;grid-row:2;width:72px;height:56px;}"
  ".right{grid-column:3;grid-row:2;width:72px;height:56px;}"
  ".down{grid-column:2;grid-row:3;width:72px;height:56px;}"
  ".stop{grid-column:2;grid-row:2;width:60px;height:60px;border-radius:50%;background:radial-gradient(circle,#ef4444,#dc2626,#991b1b);font-size:12px;box-shadow:0 5px 14px rgba(239,68,68,.45);}"
  ".speed{width:100%;max-width:370px;background:#f8fafc;border:1px solid #e5e7eb;border-radius:20px;padding:8px 12px;margin-top:7px;box-shadow:0 5px 14px rgba(0,0,0,.08);}"
  ".speedRow{display:flex;justify-content:space-between;align-items:center;font-weight:900;font-size:16px;}"
  "#speedVal{color:#16a34a;font-size:20px;}"
  "input[type=range]{width:100%;height:22px;accent-color:#16a34a;}"
  ".joyWrap{width:100%;max-width:370px;display:flex;justify-content:center;margin-top:7px;}"
  ".joyBox{width:210px;height:210px;border-radius:50%;background:radial-gradient(circle,#eef2ff,#dbeafe);border:2px solid #bfdbfe;box-shadow:inset 0 0 18px rgba(37,99,235,.12),0 8px 18px rgba(0,0,0,.12);position:relative;}"
  ".stick{width:74px;height:74px;border-radius:50%;background:linear-gradient(145deg,#2563eb,#1d4ed8);position:absolute;left:68px;top:68px;box-shadow:0 8px 18px rgba(37,99,235,.38);display:flex;align-items:center;justify-content:center;color:white;font-weight:900;font-size:12px;}"
  ".crossH{position:absolute;width:150px;height:2px;background:#93c5fd;left:30px;top:104px;}"
  ".crossV{position:absolute;width:2px;height:150px;background:#93c5fd;left:104px;top:30px;}"
  ".note{font-size:11px;color:#6b7280;margin-top:4px;}"
  "@media(max-height:760px){.title{font-size:19px}.sub{display:none}.control{grid-template-columns:66px 66px 66px;grid-template-rows:50px 50px 50px;gap:6px}.up,.left,.right,.down{width:66px;height:50px}.stop{width:54px;height:54px}.joyBox{width:185px;height:185px}.stick{width:66px;height:66px;left:59.5px;top:59.5px}.crossH{width:130px;left:27.5px;top:91.5px}.crossV{height:130px;left:91.5px;top:27.5px}.speed{padding:6px 10px;margin-top:5px}.joyWrap{margin-top:5px}.note{display:none}}"
  "</style>"
  "</head><body>"
  "<div class='app'>"
  "<div class='title'>Six Wheel Car</div>"
  "<div class='sub'>UP/DOWN latch, LEFT/RIGHT hold</div>"

  "<div class='top'>"
  "<div class='control'>"
  "<button class='btn up' ontouchstart=\"cmd('/UP')\" onclick=\"cmd('/UP')\">UP</button>"
  "<button class='btn left' ontouchstart=\"cmd('/LEFT')\" ontouchend=\"cmd('/LEFT0')\" onmousedown=\"cmd('/LEFT')\" onmouseup=\"cmd('/LEFT0')\">LEFT</button>"
  "<button class='btn stop' ontouchstart=\"cmd('/S')\" onclick=\"cmd('/S')\">STOP</button>"
  "<button class='btn right' ontouchstart=\"cmd('/RIGHT')\" ontouchend=\"cmd('/RIGHT0')\" onmousedown=\"cmd('/RIGHT')\" onmouseup=\"cmd('/RIGHT0')\">RIGHT</button>"
  "<button class='btn down' ontouchstart=\"cmd('/DOWN')\" onclick=\"cmd('/DOWN')\">DOWN</button>"
  "</div>"
  "</div>"

  "<div class='speed'>"
  "<div class='speedRow'><span>Speed</span><span id='speedVal'>140</span></div>"
  "<input type='range' min='0' max='255' value='140' oninput=\"speedVal.innerHTML=this.value;cmd('/speed?value='+this.value)\">"
  "</div>"

  "<div class='joyWrap'>"
  "<div class='joyBox' id='joy'>"
  "<div class='crossH'></div>"
  "<div class='crossV'></div>"
  "<div class='stick' id='stick'>JOY</div>"
  "</div>"
  "</div>"
  "<div class='note'>Move joystick in any direction</div>"
  "</div>"

  "<script>"
  "function cmd(u){fetch(u).catch(function(e){});}"
  "let joy=document.getElementById('joy');"
  "let stick=document.getElementById('stick');"
  "let active=false;"
  "let lastSend=0;"
  "function moveStick(x,y){"
  "let r=joy.getBoundingClientRect();"
  "let cx=r.width/2;"
  "let cy=r.height/2;"
  "let dx=x-r.left-cx;"
  "let dy=y-r.top-cy;"
  "let max=(r.width/2)-37;"
  "let dist=Math.sqrt(dx*dx+dy*dy);"
  "if(dist>max){dx=dx/dist*max;dy=dy/dist*max;}"
  "stick.style.left=(cx+dx-37)+'px';"
  "stick.style.top=(cy+dy-37)+'px';"
  "let jx=Math.round(dx/max*100);"
  "let jy=Math.round(-dy/max*100);"
  "let now=Date.now();"
  "if(now-lastSend>35){cmd('/joy?x='+jx+'&y='+jy);lastSend=now;}"
  "}"
  "function centerStick(){"
  "let r=joy.getBoundingClientRect();"
  "stick.style.left=((r.width-74)/2)+'px';"
  "stick.style.top=((r.height-74)/2)+'px';"
  "cmd('/joyStop');"
  "}"
  "joy.addEventListener('touchstart',function(e){active=true;moveStick(e.touches[0].clientX,e.touches[0].clientY);e.preventDefault();});"
  "joy.addEventListener('touchmove',function(e){if(active){moveStick(e.touches[0].clientX,e.touches[0].clientY);}e.preventDefault();});"
  "joy.addEventListener('touchend',function(e){active=false;centerStick();e.preventDefault();});"
  "joy.addEventListener('mousedown',function(e){active=true;moveStick(e.clientX,e.clientY);});"
  "document.addEventListener('mousemove',function(e){if(active){moveStick(e.clientX,e.clientY);}});"
  "document.addEventListener('mouseup',function(e){if(active){active=false;centerStick();}});"
  "</script>"

  "</body></html>";

  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);

  pinMode(L_REN, OUTPUT);
  pinMode(L_LEN, OUTPUT);
  pinMode(R_REN, OUTPUT);
  pinMode(R_LEN, OUTPUT);

  digitalWrite(L_REN, HIGH);
  digitalWrite(L_LEN, HIGH);
  digitalWrite(R_REN, HIGH);
  digitalWrite(R_LEN, HIGH);

  ledcAttach(L_RPWM, 1000, 8);
  ledcAttach(L_LPWM, 1000, 8);
  ledcAttach(R_RPWM, 1000, 8);
  ledcAttach(R_LPWM, 1000, 8);

  leftMotor(0);
  rightMotor(0);

  WiFi.softAP(ssid, password);

  server.on("/", handleRoot);

  server.on("/UP", pressUp);
  server.on("/UP0", releaseUp);

  server.on("/DOWN", pressDown);
  server.on("/DOWN0", releaseDown);

  server.on("/LEFT", pressLeft);
  server.on("/LEFT0", releaseLeft);

  server.on("/RIGHT", pressRight);
  server.on("/RIGHT0", releaseRight);

  server.on("/S", stopWeb);
  server.on("/speed", setSpeed);
  server.on("/joy", joystick);
  server.on("/joyStop", joystickStop);

  server.begin();
}

void loop() {
  server.handleClient();
  updateMotors();
}