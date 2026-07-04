# Phase 3.2 Hardware Architecture

## Implemented topology

```text
Mac desktop application
├── USB serial ── Arduino Nano
│                 ├── SSD1306 OLED
│                 ├── Joystick + switch
│                 └── Buttons 1–4
└── Wi-Fi HTTP ── ESP32-CAM (WROVER target, AI-Thinker / OV2640 pin map)
                  └── Powered from Arduino UNO 5 V rail only
```

The Nano and ESP32-CAM are independent. There is no normal-operation serial, I2C, or GPIO connection between them. The WROVER build enables PSRAM while retaining the AI-Thinker carrier's camera pins. The UNO is not a workflow controller and runs no AegisCare firmware; during normal operation it supplies USB-derived 5 V and ground only.

## Power connections

| Source | Destination |
|---|---|
| UNO `5V` | ESP32-CAM `5V` |
| UNO `GND` | ESP32-CAM `GND` |

Never use the UNO or Nano 3.3 V pin for the ESP32-CAM. Disconnect GPIO0 from ground after flashing. Keep camera power leads short and avoid powering servos or other loads from this rail.

## Phase 4 boundary

Servo-controlled positioning, HC-SR04 distance guidance, and intelligent positioning are future Phase 4 scope. No pins, drivers, placeholders, or control messages for those devices exist in Phase 3.2.
