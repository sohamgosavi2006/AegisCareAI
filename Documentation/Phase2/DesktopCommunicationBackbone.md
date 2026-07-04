# Phase 2A — Desktop Communication Backbone

## Status

Complete and build-verified. This component prepares the Qt desktop master for the Phase 2 Nano protocol extension. Phase 1 firmware and all existing reference documents remain preserved.

## Architecture

```text
Existing Qt UI
    ↓ signals / slots
CommunicationManager
    ├── QSerialPort discovery and reconnect
    ├── one-second heartbeat supervision
    ├── JsonLineProtocol (bounded framing and validation)
    └── CommunicationLogger
            ↓
       Logs/communication.log
```

The camera/TCP and simulated-screening paths remain available. Arduino connectivity is now tracked independently from camera emulation.

## Files created

- `DesktopApp/src/protocol/JsonLineProtocol.h/.cpp` — partial-read-safe newline JSON framing, compact encoding, size limits, and malformed packet rejection.
- `DesktopApp/src/communication/CommunicationLogger.h/.cpp` — timestamped TX, RX, and error logging.
- `DesktopApp/Logs/.gitkeep` — reserves the runtime log directory without storing generated logs.

## Files modified

- `DesktopApp/src/core/CommunicationManager.h/.cpp` — serial discovery, connection lifecycle, reconnect, heartbeat, JSON API, metadata, legacy Phase 1 event compatibility, and logging.
- `DesktopApp/src/ui/MainWindow.cpp` — existing Arduino HUD now shows connection color, current serial port, and firmware version when reported.
- `DesktopApp/CMakeLists.txt` — builds the new communication and protocol modules.

## Connection behavior

1. Serial ports are rescanned every two seconds.
2. Arduino, CH340, FTDI, CP210x, `usbserial`, `usbmodem`, ACM, and USB TTY candidates are supported.
3. A candidate opens at 115200 baud, 8-N-1, without flow control.
4. Desktop sends `{"type":"heartbeat","value":"PING"}` once per second.
5. Nano must answer with `{"type":"heartbeat","value":"PONG"}`.
6. A device-ready or PONG packet marks the link connected.
7. No heartbeat for 3.5 seconds closes the port; discovery then reconnects automatically.
8. Device removal, port change, malformed JSON, partial packets, and oversized packets are handled without blocking the UI.

## Verification completed

```sh
cd DesktopApp
.venv/bin/cmake -S . -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$PWD/.venv/bin/ninja" \
  -DCMAKE_PREFIX_PATH="$PWD/third_party/Qt/6.8.3/macos" \
  -DCMAKE_BUILD_TYPE=Release
.venv/bin/cmake --build build --parallel
.venv/bin/ctest --test-dir build --output-on-failure
```

Result: Qt application built successfully and the existing smoke suite passed 1/1.

## Hardware testing after Phase 2B

Do not expect the heartbeat link to become fully operational until the Nano Phase 2B protocol extension is flashed. After that component:

1. Connect only the Nano over USB.
2. Start the desktop application.
3. Confirm the HUD changes from red `ARDUINO: NC [AUTO]` to green with its port and firmware.
4. Confirm one TX PING and one RX PONG per second in `DesktopApp/Logs/communication.log`.
5. Unplug the Nano and confirm disconnection within 3.5 seconds.
6. Reconnect on another USB port and confirm automatic rediscovery.

## Next component

Phase 2B will extend the stable Nano firmware with the new typed JSON protocol, PING/PONG response, desktop-driven OLED updates, menu event packets, and firmware metadata while retaining every Phase 1 input/menu behavior.

Suggested commit message:

```text
feat(desktop): add resilient Arduino serial communication backbone
```
