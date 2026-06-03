# ESP32-6WD V7 Internet Control

Short README for the ESP32 6-wheel rover with internet control, joystick/button UI, horn buzzer, headlight toggle, speed control, trim control, and blue LED motion indication.

## Features

- Internet-based control using MQTT
- Button mode and joystick mode
- Speed slider
- Trim slider for straight driving correction
- Horn button with press / hold support
- Headlight toggle button
- Blue onboard LED turns ON only during motion command or horn sound
- Responsive HTML control page for phone and laptop viewport testing

## Files

- `internet_control.ino` → Upload to ESP32 using Arduino IDE
- `Cloud-Internet-Controller.html` → Open in browser / host on GitHub Pages

## Required Libraries

Install these in Arduino IDE:

1. `WiFi`
2. `PubSubClient`

For `PubSubClient`:
Arduino IDE → Library Manager → Search `PubSubClient` → Install

## ESP32 Motor Driver Pins

Driver pin connections are same as previous V1-6 setup.

**Left Driver**
- EN pins: GPIO 32, GPIO 33
- PWM pins: GPIO 18, GPIO 19

**Right Driver**
- EN pins: GPIO 26, GPIO 25
- PWM pins: GPIO 14, GPIO 27

Common GND is recommended between ESP32, motor drivers, battery, buzzer circuit, and headlight circuit.

## Highlighted Add-on Pins

### Horn Buzzer

**Signal pin:** GPIO 13

Use transistor switching. Do not power the buzzer directly from ESP32 GPIO.

Connection:

```text
ESP32 GPIO 13 ── 1k resistor ── NPN transistor base
Buzzer + ── 5V / VIN
Buzzer - ── NPN transistor collector
NPN transistor emitter ── GND
ESP32 GND ── Battery / supply GND
```

Recommended transistor:
- BC547 / 2N2222 / S8050

### Headlight / 5V LED Bulb

**Signal pin:** GPIO 23

Use transistor or MOSFET switching. Do not power the bulb directly from ESP32 GPIO.

Connection:

```text
Battery/VIN + ── LED bulb +
LED bulb - ── transistor/MOSFET output side
ESP32 GPIO 23 ── 1k resistor ── transistor base / MOSFET gate
Transistor/MOSFET GND side ── GND
ESP32 GND ── Battery / supply GND
```

Recommended:
- For small LED bulb: BC547 / 2N2222
- For brighter LED/light: logic-level N-MOSFET like IRLZ44N / AO3400

## Upload Steps

1. Open Arduino IDE.
2. Open the `.ino` file.
3. Select board: `ESP32 Dev Module`.
4. Install `PubSubClient` library.
5. Enter your Wi-Fi name and password in the code.
6. Upload the code to ESP32.
7. Open Serial Monitor to check Wi-Fi and MQTT connection.
8. Open the HTML file in browser.
9. Use Button / Joystick mode to control the car.
10. Use horn button for buzzer and headlight button for front light.

## HTML / GitHub Pages Steps

1. Upload the `.html` file to your GitHub repo.
2. Go to repository Settings.
3. Open Pages section.
4. Select branch and root folder.
5. Save and open the generated GitHub Pages link.
6. Open the link on phone or laptop.
7. Connect ESP32 to internet and control the rover.

## Control Behavior

- Forward / Back / Left / Right: controls rover movement
- Joystick: smooth direction control
- Speed slider: controls max speed
- Trim slider: corrects left/right drift
- Horn button:
  - Single press: quick double horn
  - Hold: continuous horn until release
- Headlight button:
  - Press once: light ON
  - Press again: light OFF

## Notes

- ESP32 GPIO cannot drive buzzer or bulb directly.
- Always use transistor or MOSFET for buzzer and headlight.
- Keep grounds common for stable operation.
- Blue LED is only for motion/horn status, not for headlight status.

## Project

**ESP32-6WD V7 Internet Control Rover**  
Made for 6-wheel ESP32 robotic car control with internet-based UI.
