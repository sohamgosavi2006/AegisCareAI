# Dependency Audit

## Repository Overview

This repository contains multiple independent projects:

- DesktopApp: Qt/C++ desktop application with CMake and optional OpenCV support.
- AegisCareAI_Embedded: Arduino/PlatformIO firmware for an AVR Nano target.
- ESP32CAM: PlatformIO/Arduino firmware for an ESP32-CAM target.
- Database: SQL schema assets.
- AIModels, Documentation, and Scripts: supporting assets and documentation.

The dependency analysis was based on the repository manifests and project configuration files that are present in the workspace:

- DesktopApp/CMakeLists.txt
- DesktopApp/requirements.txt
- AegisCareAI_Embedded/platformio.ini
- AegisCareAI_Embedded/requirements.txt
- ESP32CAM/platformio.ini
- Database/Schema/schema.sql

## Python Dependencies

No standalone Python application source files were found in the workspace. The Python dependencies below are therefore limited to declared build and tooling requirements.

Third-party Python packages detected:

- aqtinstall==3.3.0
- cmake>=3.16
- ninja
- platformio

## Desktop Application Dependencies

The desktop application is built with CMake and Qt 6.

Observed external C++ dependencies:

- Qt6 Core
- Qt6 Widgets
- Qt6 Charts
- Qt6 Sql
- Qt6 Network
- Qt6 SerialPort
- Optional Qt6 TextToSpeech support
- Optional OpenCV components: core, imgproc, objdetect
- Apple frameworks on macOS: AVFoundation, CoreMedia, CoreVideo

## Embedded Dependencies

The embedded firmware targets an Arduino-compatible AVR Nano board.

Observed requirements:

- PlatformIO project target: nanoatmega328
- Arduino framework
- U8g2 graphics library version 2.36.5 or newer

## ESP32-CAM Dependencies

The camera firmware targets an ESP32-based board using the Arduino framework.

Observed requirements:

- PlatformIO environment: esp32cam_wrover
- Platform: espressif32
- Board: esp-wrover-kit
- Framework: arduino
- Built-in ESP32 Arduino core libraries used by the source tree include WiFi, ESPmDNS, and esp_wifi
- No additional third-party PlatformIO library entries were declared in the project configuration

## PlatformIO Libraries

PlatformIO libraries declared in the repository:

- AegisCareAI_Embedded: olikraus/U8g2 @ ^2.36.5
- ESP32CAM: no external lib_deps entries declared

PlatformIO framework and board details:

- AegisCareAI_Embedded: atmelavr + nanoatmega328 + arduino
- ESP32CAM: espressif32 + esp-wrover-kit + arduino

## Arduino Libraries

Arduino libraries explicitly referenced or implied by the project configuration:

- U8g2
- WiFi
- ESPmDNS
- esp_camera
- Arduino core built-ins such as SPI and Wire

## Build Requirements

- CMake 3.16 or newer
- Ninja build generator
- Qt 6 development packages
- Optional OpenCV development packages
- PlatformIO CLI for embedded firmware projects
- Target-specific toolchains:
  - AVR-GCC for the Nano target
  - xtensa-esp32-elf for the ESP32-CAM target

## Operating System Requirements

- DesktopApp: macOS, Linux, or Windows host environments are suitable for the Qt build flow.
- macOS builds additionally require Apple framework support for camera and media integration.
- Embedded projects are cross-compiled from a host machine with PlatformIO installed.

## Compiler Requirements

- DesktopApp: C++20 capable compiler with CMake and Qt 6 support
- Embedded AVR target: AVR-GCC toolchain
- ESP32 target: ESP32 toolchain provided through PlatformIO/espressif32

## Optional Dependencies

- OpenCV for enhanced image-processing features in the desktop app
- Qt TextToSpeech for speech synthesis support
- External camera hardware and Wi-Fi connectivity for runtime operation

## Missing Dependencies

No missing Python package requirements were identified from the repository manifests. The main external build requirements that are not captured in the existing Python requirements are the platform/toolchain dependencies for Qt 6 and PlatformIO-based firmware builds.

## Installation Order

1. Install Python build tooling from the repository requirements manifest.
2. Install Qt 6 development packages and CMake/Ninja for the desktop application.
3. Install PlatformIO and its target toolchains for the embedded projects.
4. Optionally install OpenCV development packages if the desktop app is expected to use the optional vision path.

## Notes

- The SQL schema file in Database/Schema/schema.sql is currently empty, so no SQL engine extensions or version-specific requirements could be inferred from repository content.
- No source code, build logic, or project configuration files were modified during this audit; only dependency documentation was added.
