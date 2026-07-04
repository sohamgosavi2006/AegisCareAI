# Phase 3.2 Connections

## Normal operation

ESP32-CAM power:

```text
UNO 5V  ───────── ESP32-CAM 5V
UNO GND ───────── ESP32-CAM GND
```

Nano remains connected to the Mac by USB and retains the Phase 2 OLED, joystick and button wiring. Do not connect Nano RX/TX to the ESP32-CAM.

## Flashing through the Arduino UNO

```text
UNO 5V       ───── ESP32-CAM 5V
UNO GND      ───── ESP32-CAM GND
UNO TX pin 1 ───── ESP32-CAM U0R / GPIO3 RX
UNO RX pin 0 ───── ESP32-CAM U0T / GPIO1 TX
ESP32 GPIO0  ───── GND  (upload only)
```

Upload an empty sketch to the UNO before using its USB serial path. After uploading ESP32 firmware, remove GPIO0 from GND, press ESP32-CAM reset, and leave only stable 5 V/GND power connected for normal operation. TX and RX are crossed by function as shown.

## Network endpoints

| Endpoint | Purpose |
|---|---|
| `GET /stream` | Shared MJPEG stream |
| `GET /status` | JSON health and telemetry |
| `GET /heartbeat` | Lightweight availability check |
| `GET /stream/start` | Enable new stream sessions |
| `GET /stream/stop` | Stop active/new stream sessions |
| `GET /resolution?value=qvga` | Select 320×240 |
| `GET /resolution?value=vga` | Select 640×480 |
