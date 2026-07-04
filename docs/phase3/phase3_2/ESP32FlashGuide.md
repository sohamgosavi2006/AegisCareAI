# Phase 3.2 ESP32-CAM Flash Guide

1. Power off the camera.
2. Upload an empty `setup()`/`loop()` sketch to the Arduino UNO, then connect the UNO serial pins as documented in `Connections.md`.
3. Connect ESP32-CAM GPIO0 to GND.
4. Connect power and press reset.
5. Identify the upload port with `pio device list`.
   The firmware environment is `esp32cam_wrover`; its camera pin map remains AI-Thinker compatible.
6. Start upload, and press the ESP32-CAM reset button when PlatformIO displays `Connecting...`:

   ```bash
   cd ESP32CAM
   PLATFORMIO_CORE_DIR="$PWD/.platformio" ../AegisCareAI_Embedded/.venv/bin/pio run -t upload --upload-port /dev/cu.usbserial-XXXX
   ```

7. Disconnect power, remove GPIO0 from GND, restore power, and press reset.
8. Open the 115200 baud monitor and record the displayed IP.
9. Verify `http://<IP>/status` and `http://<IP>/stream` from the Mac.
10. In Workstation Settings use port `80` and click Reconnect.

During programming the UNO provides USB serial plus 5 V power. During normal operation its TX/RX and GPIO0 jumper are not required; the ESP32-CAM communicates with the desktop exclusively over Wi-Fi.
