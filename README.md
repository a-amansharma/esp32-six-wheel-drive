Here's a complete updated **README.md** you can directly copy-paste into your GitHub repository:

# 🚗 ESP32-6WD (6-Wheel Drive Rover)

My first ESP32 robotics project.

ESP32-6WD is a six-wheel-drive robotic rover built using an ESP32 development board. This project was created to learn robotics, motor control, wireless communication, embedded systems, AI, and autonomous navigation.

The objective is to continuously upgrade the same rover through multiple control technologies, starting from simple button control and progressing toward AI-powered vision and autonomous operation.

---

# 📸 Project Photos

Project photos and wiring images are stored in the `images` folder.

```txt
images/
├── car-front.jpg
├── car-side.jpg
└── wiring.jpg
```

---

# 🎯 Project Objective

The primary goal of ESP32-6WD is to explore different robotics control technologies using a single rover platform.

Instead of building multiple robots, this project upgrades the same rover through multiple versions, allowing direct comparison of control methods, performance, usability, and intelligence.

The journey begins with simple button controls and gradually progresses toward AI-based autonomous robotics.

---

# ✨ Development Roadmap

| Version | Control Method            |
| ------- | ------------------------- |
| V1      | Button Control            |
| V2      | Smooth Button Control     |
| V3      | Toggle / Hold Control     |
| V4      | Joystick Control          |
| V5      | Gyroscope Control         |
| V6      | BLE Bluefy Web Controller |
| V7      | Internet Control          |
| V8      | Voice Command Control     |
| V9      | Draw Path Control         |
| V10     | AI Command Control        |
| V11     | AI Vision Control         |

---

# 🗂 Development Versions

## V1 – Button Control

### Features

* Forward button
* Backward button
* Left button
* Right button
* Stop button

The first working version of the rover.

---

## V2 – Smooth Button Control

### Features

* Smooth acceleration
* Smooth braking
* Reduced motor jerks
* Better driving experience

Introduced motor ramping for realistic movement.

---

## V3 – Toggle / Hold Control

### Features

* One-tap continuous movement
* Hold-based steering
* Instant stop button
* Easier long-distance driving

Reduced the need to continuously press movement buttons.

---

## V4 – Joystick Control

### Features

* Full 360° joystick driving
* Variable speed control
* Smooth directional transitions
* Mobile-friendly UI
* Trim adjustment
* Speed slider

Provides natural RC-style driving.

---

## V5 – Gyroscope Control

### Features

* Tilt phone forward to move forward
* Tilt phone backward to reverse
* Tilt left/right for steering
* Motion-controlled driving

Turns a smartphone into a motion controller.

---

## V6 – BLE Bluefy Web Controller

### Features

* Bluetooth Low Energy (BLE)
* Bluefy browser support
* Direct phone-to-rover communication
* No Wi-Fi required

Provides wireless control through Bluetooth.

---

## V7 – Internet Control

### Features

* Control rover from anywhere
* Internet-based communication
* Remote operation beyond local Wi-Fi
* Long-range connectivity

Enables global rover access.

---

## V8 – Voice Command Control

### Features

* Voice-controlled navigation
* Hands-free operation
* Natural spoken commands

Example Commands:

```txt
Move Forward
Turn Left
Turn Right
Stop
```

---

## V9 – Draw Path Control

### Features

* Draw route on screen
* Rover follows drawn path
* Automatic navigation
* Waypoint-based movement

Example:

```txt
User Draws:

⬆ → → ↓ ←

Rover follows the same route automatically.
```

---

## V10 – AI Command Control

### Features

* Natural language commands
* AI-based action planning
* Multi-step instruction execution

Example:

```txt
Move forward 2 seconds and turn left.

Go to the door and stop.

Move backward and rotate right.
```

The AI converts instructions into rover actions automatically.

---

## V11 – AI Vision Control

### Features

* Phone camera acts as rover eyes
* Object detection
* Color tracking
* Face tracking
* Hand gesture tracking
* Target finding
* Object following

Example Commands:

```txt
Find the red ball.

Follow my face.

Track my hand.

Move to the bottle.
```

This is the final planned version combining AI, computer vision, and autonomous robotics.

---

# 🔌 Connection Map

## Left Motor Driver

```txt
ESP32            LEFT DRIVER
--------------------------------
VIN / 3V3  --->  VCC
GND        --->  GND

D32        --->  L_EN
D33        --->  R_EN

D18        --->  L_PWM
D19        --->  R_PWM
```

## Right Motor Driver

```txt
ESP32            RIGHT DRIVER
--------------------------------
VIN / 3V3  --->  VCC
GND        --->  GND

D26        --->  L_EN
D25        --->  R_EN

D14        --->  L_PWM
D27        --->  R_PWM
```

---

# 🔧 Hardware Used

* ESP32 Development Board
* Six DC Geared Motors
* Dual Motor Driver Modules
* 6WD Rover Chassis
* Li-Ion Battery Pack
* Jumper Wires
* Smartphone
* Power Switch

Future Versions:

* Camera Module
* AI Vision System
* Voice Recognition
* Cloud Connectivity

---

# 📁 Repository Structure

```txt
ESP32-6WD/
│
├── README.md
├── LICENSE
├── images/
│
├── V1-button-control/
├── V2-smooth-button-control/
├── V3-toggle-hold-control/
├── V4-joystick-control/
├── V5-gyroscope-control/
├── V6-ble-bluefy-controller/
├── V7-internet-control/
├── V8-voice-command-control/
├── V9-draw-path-control/
├── V10-ai-command-control/
└── V11-ai-vision-control/
```

---

# 🚀 Future Goal

Transform ESP32-6WD from a simple remote-controlled rover into a fully AI-powered autonomous robotic platform capable of understanding commands, seeing the environment, identifying targets, and performing intelligent navigation.

---

# 👨‍💻 Author

**Aman Sharma**

Certified Curious Coder

BITS Pilani (B.Sc. Computer Science)

Robotics • Embedded Systems • AI • IoT

---

# ☕ Let's Build Something Cool

Technology is more fun when shared.

Whether it's robotics, AI, embedded systems, crazy project ideas, or simply a good conversation, feel free to reach out.

📷 Instagram: **@_ar.sharma**

> "Let's talk robots, build something awesome, or just say hi."

🔗 https://instagram.com/_ar.sharma

---

Thanks for visiting the project.

See you in the next build. 🚀
