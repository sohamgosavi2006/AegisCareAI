# Phase 3.2 PlatformIO Build Guide

## Configure Wi-Fi

```bash
cd ESP32CAM
cp include/Secrets.example.h include/Secrets.h
```

Edit `include/Secrets.h` with the Wi-Fi used by the Mac. `Secrets.h` should not be committed. If it is absent or station connection times out, firmware creates:

```text
SSID: AegisCare-CAM
Password: AegisCare2026
IP: 192.168.4.1
```

## Build

From the project root:

```bash
cd ESP32CAM
PLATFORMIO_CORE_DIR="$PWD/.platformio" ../AegisCareAI_Embedded/.venv/bin/pio run
```

The `esp32cam_wrover` environment targets the ESP32 WROVER module with 4 MB flash, PSRAM, QIO flash mode and the AI-Thinker camera pin map. The first build downloads the official Espressif PlatformIO toolchain. Subsequent builds use `ESP32CAM/.platformio`.

## Serial monitor

```bash
PLATFORMIO_CORE_DIR="$PWD/.platformio" ../AegisCareAI_Embedded/.venv/bin/pio device monitor -b 115200
```

Expected boot sequence includes `BOOT`, `CAMERA Started`, `WIFI Connected` or `WIFI AP Ready`, and `STREAM Server ready`.
