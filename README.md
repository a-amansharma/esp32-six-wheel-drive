# 🚗 ESP32-6WD (6-Wheel-Drive)

My first ESP32 project.

ESP32-6WD is a six-wheel-drive (6WD) robotic car built using an ESP32 development board. This project was created to learn robotics, motor control, wireless communication, and embedded systems programming.

The main goal of this project is to control a single ESP32-based six-wheel-drive vehicle using multiple control methods. Each version explores a different approach, ranging from basic Wi-Fi control to internet-based remote operation.

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

# 🎯 Project Objective

The primary goal of ESP32-6WD is to develop and test multiple control systems for a single six-wheel-drive robotic vehicle using ESP32.

Instead of building different robots, this project focuses on implementing different control methods for the same vehicle and comparing their performance, usability, and driving experience.

The project is organized into separate versions, where each version represents a different way of controlling the same 6WD car.

---

# ✨ Control Methods

ESP32-6WD is a single six-wheel-drive robot car that is being developed with multiple control methods.

## Control Methods Included in This Project

1. Basic Wi-Fi Control
2. Smooth Wi-Fi Control
3. Button Hold & Release Control
4. Virtual Joystick Control
5. Phone Gyroscope Control
6. Bluetooth Control
7. Home Router Wi-Fi Control
8. Internet Control

---

# 🗂 Development Versions

This project is organized into multiple versions, where each version implements a different method of controlling the ESP32-6WD vehicle.

## Version 01 – Basic Wi-Fi Control

**Folder**

```txt
codes/01-basic-wifi-control/
```

**Features**

- Forward button
- Backward button
- Left button
- Right button
- Stop button

This was the first working version of the ESP32-6WD car.

---

## Version 02 – Smooth Wi-Fi Control

**Folder**

```txt
codes/02-smooth-wifi-control/
```

**Features**

- Smooth acceleration
- Smooth deceleration
- Improved driving experience

This version reduced sudden starts and stops.

---

## Version 03 – Button Hold & Release Control

**Folder**

```txt
codes/03-button-hold-release/
```

**Features**

- UP button remains active after pressing
- DOWN button remains active after pressing
- LEFT button works only while holding
- RIGHT button works only while holding
- STOP button immediately stops the vehicle

This version introduced more natural steering behavior.

---

## Version 04 – Virtual Joystick Control

**Folder**

```txt
codes/04-joystick-control/
```

**Features**

- Virtual joystick interface
- Full directional control
- Adjustable speed slider
- Button controls retained
- Smooth motor ramping

This is currently the most advanced working version.

---

## Version 05 – Phone Gyroscope Control

**Folder**

```txt
codes/05-gyroscope-control-future/
```

**Features**

- Tilt phone forward to move forward
- Tilt phone backward to move backward
- Tilt phone left to turn left
- Tilt phone right to turn right

**Status:** Planned

---

## Version 06 – Bluetooth Control

**Folder**

```txt
codes/06-bluetooth-control-future/
```

**Features**

- ESP32 Bluetooth communication
- Direct phone-to-car connection
- Wireless control without Wi-Fi

**Status:** Planned

---

## Version 07 – Home Router Wi-Fi Control

**Folder**

```txt
codes/07-home-router-wifi-future/
```

**Features**

- Connect ESP32-6WD to home Wi-Fi
- Access the control interface through the local network
- Extended operating range

**Status:** Planned

---

## Version 08 – Internet Control

**Folder**

```txt
codes/08-internet-control-future/
```

**Features**

- Control the car from anywhere
- Cloud connectivity
- Remote access over the internet

**Status:** Planned

---

# 🔧 Hardware Used

- ESP32 Development Board
- Six DC Motors
- Dual Motor Driver Modules
- Battery Pack
- 6WD Chassis
- Jumper Wires
- Smartphone for Control

---

# 📁 Repository Structure

```txt
ESP32-6WD/
│
├── README.md
├── LICENSE
├── .gitignore
│
├── images/
│   ├── car-front.jpg
│   ├── car-side.jpg
│   └── wiring.jpg
│
└── codes/
    ├── 01-basic-wifi-control/
    ├── 02-smooth-wifi-control/
    ├── 03-button-hold-release/
    ├── 04-joystick-control/
    ├── 05-gyroscope-control-future/
    ├── 06-bluetooth-control-future/
    ├── 07-home-router-wifi-future/
    └── 08-internet-control-future/
```

---

# 👨‍💻 Author

**Aman Sharma**

First ESP32 Project – ESP32-6WD

---

# ⭐ Support

If you found this project interesting, consider giving the repository a star and following future updates.