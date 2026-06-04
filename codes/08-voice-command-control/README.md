# ESP32 6WD Rover - Siri Voice Control Edition

A Wi-Fi based 6WD (Six Wheel Drive) rover powered by ESP32, featuring smooth motor control, voice commands through Apple Siri, headlights, horn effects, pulse modes, speed presets, and timed movement commands.

## Features

* ESP32 Wi-Fi Access Point Control
* Apple Siri Voice Commands
* Smooth Acceleration & Deceleration
* 6WD Differential Drive
* Headlight Control
* Horn Control
* Pulse Headlight Mode
* Pulse Horn Mode
* Speed Presets
* Timed Forward Movement
* Timed Backward Movement
* Emergency Stop Command
* No Additional Mobile App Required

---

# How It Works

The ESP32 creates its own Wi-Fi hotspot.

The iPhone connects directly to the ESP32 hotspot.

Apple Shortcuts are configured with simple HTTP URLs.

When a Siri command is spoken, Siri runs the corresponding shortcut and sends the command directly to the ESP32.

Siri → Shortcut → URL Request → ESP32 → Rover Action

---

# Setup Instructions

## Step 1: Upload the Code

1. Open Arduino IDE.
2. Select ESP32 board.
3. Upload the provided firmware.
4. Wait for successful upload.

---

## Step 2: Connect to ESP32 Wi-Fi

On iPhone:

Settings → Wi-Fi

Connect to:

ESP32-6WD

Password:

12345678

Wait until the connection is established.

---

## Step 3: Create a Shortcut

1. Open the Shortcuts app.
2. Tap "+".
3. Tap "Add Action".
4. Search for "URL".
5. Add URL action.
6. Paste the command URL.
7. Add "Get Contents of URL".
8. Method = GET.
9. Rename the shortcut.
10. Add Siri phrase.

The shortcut is now ready.

---

# Voice Commands

## Movement

### Move Forward

Siri Phrase:

Move Forward

URL:

http://192.168.4.1/cmd?c=F

---

### Move Backward

Siri Phrase:

Move Backward

URL:

http://192.168.4.1/cmd?c=B

---

### Stop Car

Siri Phrase:

Stop Car

URL:

http://192.168.4.1/cmd?c=S

This command immediately stops:

* Motors
* Headlights
* Pulse Headlights
* Pulse Horn
* Horn

---

## Rotation Commands

### Rotate Left

Siri Phrase:

Rotate Left

URL:

http://192.168.4.1/cmd?c=RL

Continuous rotation until stopped.

---

### Rotate Right

Siri Phrase:

Rotate Right

URL:

http://192.168.4.1/cmd?c=RR

Continuous rotation until stopped.

---

## Quick Turn Commands

### Turn Left

Siri Phrase:

Turn Left

URL:

http://192.168.4.1/cmd?c=TL

Performs a short turn and stops automatically.

---

### Turn Right

Siri Phrase:

Turn Right

URL:

http://192.168.4.1/cmd?c=TR

Performs a short turn and stops automatically.

---

# Speed Presets

### Speed 25

http://192.168.4.1/cmd?c=V25

### Speed 50

http://192.168.4.1/cmd?c=V50

### Speed 75

http://192.168.4.1/cmd?c=V75

### Speed 100

http://192.168.4.1/cmd?c=V100

Speed changes instantly, even while the rover is already moving.

---

# Timed Forward Movement

### Forward One Second

http://192.168.4.1/cmd?c=F1

### Forward Two Seconds

http://192.168.4.1/cmd?c=F2

### Forward Three Seconds

http://192.168.4.1/cmd?c=F3

---

# Timed Backward Movement

### Backward One Second

http://192.168.4.1/cmd?c=B1

### Backward Two Seconds

http://192.168.4.1/cmd?c=B2

### Backward Three Seconds

http://192.168.4.1/cmd?c=B3

---

# Horn Commands

### Horn

Siri Phrase:

Horn

URL:

http://192.168.4.1/cmd?c=HORN

Beeps for one second.

---

### Pulse Horn

Siri Phrase:

Pulse Horn

URL:

http://192.168.4.1/cmd?c=PHON

Repeating horn pattern.

---

### Horn Off

Siri Phrase:

Horn Off

URL:

http://192.168.4.1/cmd?c=PHOFF

Stops pulse horn.

---

# Headlight Commands

### Headlight On

Siri Phrase:

Headlight On

URL:

http://192.168.4.1/cmd?c=LON

---

### Headlight Off

Siri Phrase:

Headlight Off

URL:

http://192.168.4.1/cmd?c=LOFF

Turns off both normal and pulse lighting modes.

---

### Pulse Headlight

Siri Phrase:

Pulse Headlight

URL:

http://192.168.4.1/cmd?c=PLON

Repeating flashing pattern.

---

# Recommended Siri Commands

Move Forward

Move Backward

Rotate Left

Rotate Right

Turn Left

Turn Right

Speed 25

Speed 50

Speed 75

Speed 100

Forward One Second

Forward Two Seconds

Forward Three Seconds

Backward One Second

Backward Two Seconds

Backward Three Seconds

Horn

Pulse Horn

Horn Off

Headlight On

Headlight Off

Pulse Headlight

Stop Car

---

# Future Upgrades

* MPU6050 Angle Based Turning
* Distance Based Navigation
* Voice Controlled Speed Input
* Voice Controlled Angle Input
* AI Navigation
* Camera Streaming
* Object Detection
* Autonomous Patrol Mode
* Robotic Arm Integration

---

## Author

Aman Sharma

Certified Curious Coder

Built with ESP32, Arduino IDE, Apple Shortcuts, and a lot of robotics enthusiasm.
