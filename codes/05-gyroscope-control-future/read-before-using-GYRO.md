# Phone Tilt Control (Gyroscope Version)

This version allows the rover to be controlled by tilting a smartphone.

Originally, I tried reading motion sensors directly from the ESP32-hosted web page. While it may work on some Android devices, it was not reliable on iPhone due to browser sensor restrictions. To solve this, sensor data is read using the **phyphox** app and sent to the ESP32.

## iPhone Setup

1. Install **phyphox** from the App Store.
2. Connect the iPhone to the ESP32 WiFi.
3. Open phyphox.
4. Go to **Raw Sensors → Acceleration with g**.
5. Press the Start button.
6. Open the menu and select **Allow Remote Access**.
7. Keep phyphox open in the foreground.
8. Hold the phone in your normal driving position.
9. Press the ESP32 **EN** button once to calibrate the center position.
10. Tilt the phone to drive the rover.

## Android Setup

### Method 1 (Recommended)

1. Install **phyphox**.
2. Connect the phone to the ESP32 WiFi.
3. Open **Raw Sensors → Acceleration with g**.
4. Press Start.
5. Enable **Remote Access**.
6. Press the ESP32 **EN** button once for calibration.
7. Tilt the phone to drive.

### Method 2

Some Android phones allow motion sensors directly inside the ESP32 web page.

1. Connect to the ESP32 WiFi.
2. Open the control page.
3. Start the gyroscope mode.
4. Allow motion sensor permissions if requested.
5. Calibrate and drive.

## Notes

* Use **Acceleration with g**.
* Do **not** use **Gyroscope Rotation Rate** for tilt driving.
* Keep phyphox running while driving.
* If steering direction is reversed, invert the corresponding axis in the code.
