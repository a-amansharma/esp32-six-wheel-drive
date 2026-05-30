# 🚗 ESP32-6WD (6-Wheel-Drive)

My first ESP32 project.

ESP32-6WD is a six-wheel drive (6WD) car built using an ESP32 development board. The project was created to learn robotics, motor control, wireless communication, and embedded systems programming.

The goal of this project is to experiment with different control methods and continuously improve the vehicle by adding new features and control systems.

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
# Connection Map

### ESP32                  LEFT DRIVER
--------------------------------------
VIN / 3V3  --------->  VCC

GND        --------->  GND

D32        --------->  L_EN

D33        --------->  R_EN

D18        --------->  L_PWM

D19        --------->  R_PWM

_______________________________________
### ESP32                  RIGHT DRIVER
--------------------------------------
VIN / 3V3  --------->  VCC

GND        --------->  GND

D26        --------->  L_EN

D25        --------->  R_EN

D14        --------->  L_PWM

D27        --------->  R_PWM



# 🎯 Project Objectives

This project was built to explore different ways of controlling a 6WD car using ESP32.

Current and future objectives include:

* Wi-Fi control
* Smooth motor control
* Hold-and-release steering
* Joystick control
* Phone gyroscope control
* Bluetooth control
* Home Wi-Fi router control
* Internet-based control

---

# ✨ Features

### Current Features

✅ ESP32 based 6WD car

✅ Phone Wi-Fi control

✅ Basic directional control

✅ Smooth acceleration and deceleration

✅ Hold-and-release steering

✅ Virtual joystick control

✅ Adjustable speed control

---

### Future Features

🔄 Phone gyroscope control

🔄 Bluetooth control

🔄 Home Wi-Fi router connectivity

🔄 Internet control from anywhere

🔄 Voice control

🔄 Camera integration

---

# 🗂 Development Versions

This project has evolved through multiple versions as new features were added.

---

## Version 01 – Basic Wi-Fi Control

Folder:

```txt
codes/01-basic-wifi-control/
```

Features:

* Forward button
* Backward button
* Left button
* Right button
* Stop button

This was the first working version of the ESP32-6WD car.

---

## Version 02 – Smooth Wi-Fi Control

Folder:

```txt
codes/02-smooth-wifi-control/
```

Features:

* Smooth acceleration
* Smooth deceleration
* Improved driving experience

This version reduced sudden starts and stops.

---

## Version 03 – Button Hold & Release Control

Folder:

```txt
codes/03-button-hold-release/
```

Features:

* UP button remains active after pressing
* DOWN button remains active after pressing
* LEFT button works only while holding
* RIGHT button works only while holding
* STOP button immediately stops the vehicle

This version introduced more natural steering behavior.

---

## Version 04 – Joystick Control

Folder:

```txt
codes/04-joystick-control/
```

Features:

* Virtual joystick interface
* Full directional control
* Adjustable speed slider
* Button controls retained
* Smooth motor ramping

This is currently the most advanced working version.

---

## Version 05 – Gyroscope Control (Planned)

Folder:

```txt
codes/05-gyroscope-control-future/
```

Planned Features:

* Tilt phone forward → move forward
* Tilt phone backward → move backward
* Tilt phone left → turn left
* Tilt phone right → turn right

Status: Planned.

---

## Version 06 – Bluetooth Control (Planned)

Folder:

```txt
codes/06-bluetooth-control-future/
```

Planned Features:

* ESP32 Bluetooth communication
* Direct phone-to-car connection
* Wireless control without Wi-Fi

Status: Planned.

---

## Version 07 – Home Router Wi-Fi Control (Planned)

Folder:

```txt
codes/07-home-router-wifi-future/
```

Planned Features:

* Connect ESP32-6WD to home Wi-Fi
* Access the control interface through the local network
* Extended operating range

Status: Planned.

---

## Version 08 – Internet Control (Planned)

Folder:

```txt
codes/08-internet-control-future/
```

Planned Features:

* Control the car from anywhere
* Cloud connectivity
* Remote access over the internet

Status: Planned.

---

# 🔧 Hardware Used

* ESP32 Development Board
* Six DC Motors
* Motor Driver Module
* Battery Pack
* 6WD Chassis
* Jumper Wires
* Smartphone for Control

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

# 🚀 Future Roadmap

* Gyroscope driving
* Bluetooth control
* Router-based control
* Internet control
* Voice control
* Camera streaming
* Autonomous navigation

---

# 👨‍💻 Author

**Aman Sharma**

First ESP32 Project – ESP32-6WD

---

# ⭐ Support

If you found this project interesting, feel free to star the repository and follow future updates.
