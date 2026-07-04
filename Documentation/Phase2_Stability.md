# Phase 2 Stability and Patient Synchronization

## Scope

This phase stabilizes the existing Desktop ↔ Arduino Nano USB serial link and synchronizes the existing patient registry with the Nano OLED. It does not add ESP32-CAM, AI, camera, database, or workflow features. Previous documentation remains unchanged.

## Stability architecture

- `CommunicationManager::instance()` remains the only manager instance.
- One `QSerialPort` is allocated once in the singleton constructor and reused.
- `startAll()` is idempotent and cannot start duplicate reconnect loops.
- An open or opening port is never reopened.
- DTR and RTS are held inactive after the initial open to avoid repeated Nano reset pulses.
- The desktop sends one PING per second only after the link is established.
- Nano answers PONG immediately from its non-blocking loop.
- Five seconds without DEVICE_READY/PONG closes the failed link once.
- Reconnect uses one guarded single-shot timer with 1, 2, 5, and 10 second delays; 10 seconds is the maximum.
- A successful heartbeat resets reconnect backoff.
- Partial JSON remains buffered. Malformed and oversized JSON is logged and ignored without closing the port.
- Only resource removal, missing device, permission/open failure, or heartbeat timeout can initiate reconnect. Transient framing/parity/read/write errors are warnings.
- The boot/loading sequence is called only by Arduino `setup()`. Reconnect does not invoke application startup logic; eliminating repeated port opens prevents repeated reset/loading cycles.

## Patient synchronization

The desktop registry is authoritative. It sends a four-record sliding window around the selected patient, allowing a registry of any desktop-supported size while respecting the Nano's 2 KB SRAM.

```json
{"type":"patient_list","total":6,"patients":[
  {"index":1,"id":"AEGIS-2026-0002","name":"Patient Two"},
  {"index":2,"id":"AEGIS-2026-0003","name":"Patient Three"}
]}
```

Desktop selection:

```json
{"type":"patient_selected","index":4,"id":"AEGIS-2026-0005","name":"Vijay Deshmukh"}
```

Nano navigation and confirmation:

```json
{"type":"patient_changed","index":5}
{"type":"patient_confirm"}
```

The Nano parser consumes `patient_list` incrementally rather than buffering the complete packet. It stores four compact patient records, their global indexes, the registry total, abbreviated OLED names, and four-character ID suffixes. Moving beyond the local window asks the desktop for the adjacent global index; the desktop selects and scrolls that row, updates details and QR, then sends a refreshed window.

## Expected OLED behavior

```text
        AegisCare AI
----------------------
^  Vijay Deshmukh
   ID: 0005
   Patient 5 / 6       v
```

UP selects the previous patient, DOWN selects the next, joystick CLICK confirms, and LEFT returns to the menu. One completed frame is rendered per state change; idle loops do not redraw.

## Expected log sequence

`DesktopApp/Logs/communication.log` uses timestamped records:

```text
2026-07-01 10:10:01.010 CONNECT Opened cu.usbserial-110; awaiting heartbeat
2026-07-01 10:10:01.950 RX {"type":"device","status":"DEVICE_READY","firmware":"2.1.0"}
2026-07-01 10:10:01.951 CONNECT Heartbeat established on cu.usbserial-110
2026-07-01 10:10:02.950 TX {"type":"heartbeat","value":"PING"}
2026-07-01 10:10:02.955 RX {"type":"heartbeat","value":"PONG"}
2026-07-01 10:11:20.000 WARNING Missing heartbeat for 5000 ms on cu.usbserial-110
2026-07-01 10:11:20.001 RECONNECT Missing heartbeat; retry in 1000 ms
```

## Automated tests completed

Desktop:

```sh
cd DesktopApp
.venv/bin/cmake -S . -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$PWD/.venv/bin/ninja" \
  -DCMAKE_PREFIX_PATH="$PWD/third_party/Qt/6.8.3/macos" \
  -DCMAKE_BUILD_TYPE=Release
.venv/bin/cmake --build build --parallel
.venv/bin/ctest --test-dir build --output-on-failure
```

Expected: `communication_protocol` and `core_smoke` both pass. The protocol test covers partial frames, malformed JSON, oversized frames, recovery, newline termination, and type-first encoding.

Nano:

```sh
cd AegisCareAI_Embedded
PLATFORMIO_CORE_DIR="$PWD/.platformio" .venv/bin/pio run
```

Verified build: 23,194 bytes flash (75.5%) and 1,405 bytes SRAM (68.6%), leaving 643 bytes of SRAM. The U8g2 single-page framebuffer is included in this reported SRAM figure; there is no hidden 1 KB full-frame allocation.

## Physical test procedure

No Nano was visible during automated verification; only macOS Bluetooth serial devices were present. Perform these tests after connecting and flashing the physical Nano:

1. Close Arduino Serial Monitor and every program that could own the Nano port.
2. Upload firmware:

   ```sh
   cd AegisCareAI_Embedded
   PLATFORMIO_CORE_DIR="$PWD/.platformio" .venv/bin/pio run --target upload
   ```

3. Disconnect/reconnect the Nano once, then launch `DesktopApp/run-aegiscare.sh`.
4. Confirm the Nano boot/loading sequence appears once and the desktop HUD becomes green within five seconds.
5. Observe logs for one PING/PONG pair per second for at least five minutes. Expected: no disconnect, reconnect, or repeated loading.
6. Send malformed text such as `{broken}\n` from a serial test harness. Expected: ERROR log, no disconnect.
7. Unplug the Nano. Expected: red status after five seconds and exactly one scheduled reconnect at a time.
8. Leave unplugged. Expected reconnect delays: 1 s, 2 s, 5 s, then 10 s repeatedly.
9. Reconnect on another USB port. Expected: automatic discovery, one boot sequence, green status, backoff reset.
10. Open Patient Registry and click a row. Expected: matching OLED name, ID suffix, and position.
11. Move joystick UP/DOWN. Expected within 100 ms: table row selection, automatic scroll, preview, QR, details, and OLED selection remain aligned.
12. Click the joystick. Expected: `patient_confirm`, active patient loaded, no reconnect.
13. Test the first and last registry rows. Expected: no underflow/overflow and only valid arrows shown.
14. Apply search/village filters and repeat. Expected: Nano navigates the currently visible table ordering.

## Failure cases

| Failure | Expected behavior |
|---|---|
| Partial JSON | Buffered until newline; no action and no disconnect |
| Malformed JSON | Logged and ignored; next valid packet parses |
| Packet over 512 bytes on desktop RX | Discarded through newline; parser recovers |
| Nano patient list larger than serial buffer | Stream-parsed without full-packet allocation |
| Heartbeat delayed under five seconds | Link remains connected |
| Heartbeat absent for five seconds | One close and one scheduled reconnect |
| Port removed or renamed | Exponential rediscovery without manager/port recreation |
| Repeated `startAll()` call | No duplicate timers or serial open |
| Joystick at first/last patient | Index remains within registry bounds |

## Timing targets

- PING interval: 1,000 ms.
- PONG: next Nano loop iteration, normally under 10 ms.
- Heartbeat timeout: 5,000 ms.
- Reconnect delays: 1,000 / 2,000 / 5,000 / 10,000 ms.
- A four-patient window at 115200 baud is normally transmitted in under 30 ms.
- Patient selection UI/OLED synchronization target: under 100 ms.

## Files created

- `AegisCareAI_Embedded/include/PatientSync.h`
- `AegisCareAI_Embedded/src/PatientSync.cpp`
- `DesktopApp/tests/CommunicationProtocolTests.cpp`
- `Documentation/Phase2_Stability.md`

## Files modified

- `DesktopApp/src/core/CommunicationManager.h/.cpp`
- `DesktopApp/src/communication/CommunicationLogger.h/.cpp`
- `DesktopApp/src/protocol/JsonLineProtocol.cpp`
- `DesktopApp/src/ui/PatientManagementView.h/.cpp`
- `DesktopApp/CMakeLists.txt`
- `AegisCareAI_Embedded/include/Communication.h`
- `AegisCareAI_Embedded/include/Display.h`
- `AegisCareAI_Embedded/src/Communication.cpp`
- `AegisCareAI_Embedded/src/Display.cpp`
- `AegisCareAI_Embedded/src/main.cpp`
- `AegisCareAI_Embedded/platformio.ini`

## Known limitations

- Physical USB stability and the under-100-ms end-to-end target require the connected Nano and cannot be proven by compilation alone.
- The Nano OLED uses a compact ASCII font. Names are limited to 14 displayed characters; non-ASCII characters may appear as `?`. Full names remain unchanged in the desktop database and UI.
- The Nano keeps four patients in SRAM at once, but global indexes and sliding windows allow navigation across the full visible desktop registry.
- macOS port ownership is exclusive. PlatformIO monitor, Arduino IDE Serial Monitor, and the desktop application cannot use the same port simultaneously.

Suggested commit message:

```text
fix(serial): stabilize Nano heartbeat and synchronize patient selection
```
