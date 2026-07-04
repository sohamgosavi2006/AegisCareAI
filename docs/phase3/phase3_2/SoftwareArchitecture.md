# Phase 3.2 Software Architecture

## ESP32-CAM firmware

- `Config`: firmware identity, network defaults, camera pins, resolution and performance limits.
- `WiFiManager`: station connection, access-point fallback, bounded reconnection and mDNS advertisement.
- `CameraManager`: one OV2640 instance supporting QVGA/VGA JPEG capture.
- `StreamServer`: one HTTP server exposing MJPEG and lightweight control/status routes.
- `StatusManager`: FPS, frame count, bytes, uptime and JSON telemetry.
- `Logger`: timestamped boot, network, camera, client, stream and error messages.
- `Utilities`: wrap-safe timing helpers.

`main.cpp` performs composition and lifecycle supervision only. It contains no camera, Wi-Fi, or HTTP implementation.

## Desktop integration

`CameraStreamManager` remains the single shared camera service. Its worker thread:

1. Tries the configured address, `aegiscare-cam.local`, then `192.168.4.1`.
2. Opens one `/stream` MJPEG connection.
3. Decodes complete JPEG frames away from the GUI thread.
4. Polls `/status` every two seconds.
5. Publishes the same frame to Workstation Settings and Assisted Screening.
6. Reconnects with bounded 1, 2, 5 and 10 second delays.

Malformed status JSON, unavailable endpoints and incomplete JPEG frames do not crash or block the UI.

