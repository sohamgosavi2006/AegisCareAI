# Phase 3.2 Network Configuration

## Preferred station mode

Place the Mac and ESP32-CAM on the same 2.4 GHz Wi-Fi network. The ESP32-CAM does not support 5 GHz-only networks. The firmware advertises `aegiscare-cam.local` through mDNS and the desktop automatically tries:

1. The address saved in Workstation Settings.
2. `aegiscare-cam.local`.
3. `192.168.4.1`.

## Access-point fallback

If station credentials are absent or unavailable, join `AegisCare-CAM` from the Mac and use `192.168.4.1`, port `80`. Internet access is not required.

## Status response

`GET /status` reports camera/Wi-Fi state, RSSI, IP, FPS, frame count, streaming state, resolution, uptime, firmware version, heap and transmitted bytes. Workstation Settings polls it every two seconds without blocking the GUI.

For reliable demonstrations, keep the ESP32-CAM close to the access point and avoid guest networks that isolate clients from each other.

