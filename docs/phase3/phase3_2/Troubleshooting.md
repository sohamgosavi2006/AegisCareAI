# Phase 3.2 Troubleshooting

| Symptom | Check |
|---|---|
| Brownout/reset loop | Use ESP32-CAM `5V`, not `3.3V`; shorten wires; disconnect other UNO loads. |
| Upload timeout | GPIO0 must be grounded during upload; TX/RX must cross; press reset after upload begins. |
| Boots into download mode | Remove GPIO0 from GND after flashing and reset. |
| Camera initialization fails | Confirm AI-Thinker board and OV2640 ribbon seating; inspect serial error code. |
| Wi-Fi timeout | Use 2.4 GHz credentials; verify `Secrets.h`; otherwise join fallback AP. |
| `.local` does not resolve | Enter the serial-monitor IP directly in Workstation Settings. |
| Stream connects but no frames | Open `/status`; confirm `camera_online` and `streaming` are true; call `/stream/start`. |
| Repeated stream disconnects | Improve power and RSSI; use QVGA; ensure only the AegisCare shared stream is open. |
| Desktop remains offline | Use port 80 for Phase 3.2 firmware and click Reconnect. |

The firmware retries camera initialization and Wi-Fi without intentionally rebooting. A repeated hardware brownout is a power problem, not an application reconnect condition.

