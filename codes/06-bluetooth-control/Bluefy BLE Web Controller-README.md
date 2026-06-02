# ESP32 6WD BLE Web Controller (Bluefy)

## Overview

Control the ESP32 6WD Rover using a web-based Bluetooth controller.

Features:

* Forward, Backward, Left, Right, Stop
* Speed Control (0–255)
* BLE UART Communication
* Works on iPhone and Android
* No dedicated app development required

---

## Required App

### iPhone

* Install **Bluefy – Web BLE Browser**

### Android

* Use **Google Chrome** (recommended)
* Most modern Android browsers support Web Bluetooth

---

## BLE Commands

| Action   | Command |
| -------- | ------- |
| Forward  | B       |
| Backward | F       |
| Left     | R       |
| Right    | L       |
| Stop     | S       |
| Speed    | V0–V255 |

*Direction commands are mapped according to the motor wiring used during testing.*

---

## GitHub Pages Setup

1. Create a GitHub repository.
2. Upload the provided `index.html`.
3. Push the code to GitHub.
4. Open **Settings → Pages**.
5. Select:

   * Source: **Deploy from a branch**
   * Branch: **main**
   * Folder: **/root**
6. Save.

GitHub will generate a link similar to:

https://a-amansharma.github.io/ESP32-6WD-BLE-Web/

---

## How to Use

### iPhone (Bluefy)

1. Power ON the ESP32 rover.

2. Open Bluefy.

3. Open the GitHub Pages link.

4. Tap **Connect Bluetooth**.

5. Select the ESP32 device.

6. Wait for:

   Connected: ESP32_6WD_BLE

7. Use the on-screen controls.

### Android (Chrome)

1. Power ON the ESP32 rover.
2. Open the GitHub Pages link in Chrome.
3. Tap **Connect Bluetooth**.
4. Select the ESP32 device.
5. Wait for connection.
6. Use the on-screen controls.

---

## Notes

* iPhone users must open the page inside Bluefy.
* Android users should use Chrome.
* Disconnect other BLE apps before connecting.
* Keep the ESP32 BLE firmware uploaded to the board.
* Refresh the page and reconnect if Bluetooth fails.

---

## Version

**V6 – Bluefy BLE Web Controller**
