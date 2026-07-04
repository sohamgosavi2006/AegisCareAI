# Camera Source Fallback Architecture

## Purpose

ESP32-CAM remains the preferred scan provider and all Phase 3.2 firmware, HTTP routes, networking, telemetry and settings remain intact. This addition only changes desktop behavior while ESP32-CAM is unreachable.

## Provider boundary

```text
CameraSourceManager
├── ESP32CameraProvider ── existing CameraStreamManager ── ESP32-CAM /stream
└── LaptopCameraProvider ── macOS AVFoundation ── built-in/system webcam
                │
                └── QImage frameReady
                         ↓
                ScreeningView::handleFrameReceived
                         ↓
                existing quality, face and scan pipeline
```

No analysis logic is duplicated inside either provider. Both emit owned `QImage` frames through the same source-manager signal.

## Auto mode

1. Start the existing ESP32 service and discovery sequence.
2. If a valid ESP32 frame arrives, use ESP32-CAM and stop laptop capture.
3. If no ESP32 frame arrives within 2.8 seconds, start the laptop camera without restarting the application.
4. Continue probing ESP32 while fallback is active.
5. When ESP32 frames return, promote ESP32 automatically and release the laptop camera.
6. If an active ESP32 stream stops producing frames for four seconds, immediately return to laptop fallback.

Settings can explicitly select Auto, ESP32-CAM, or Laptop Webcam. Auto is the persisted default.

## macOS privacy

The application bundle contains `NSCameraUsageDescription`. macOS asks for camera access the first time laptop fallback is required. If access is denied, the UI reports it through a non-blocking toast and keeps the ESP32 retry service intact.

## Preserved components

- ESP32 WROVER PlatformIO firmware and AI-Thinker pin configuration
- MJPEG stream, status and heartbeat APIs
- `CameraStreamManager` worker and reconnect logic
- ESP32 telemetry and diagnostics
- database, patient, authentication and workflow architecture
- Phase 4 compatibility boundary

