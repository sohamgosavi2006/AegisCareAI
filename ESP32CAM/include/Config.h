#pragma once

#include <Arduino.h>
#include <esp_camera.h>

#if __has_include("Secrets.h")
#include "Secrets.h"
#endif

namespace AegisCare::Config {

inline constexpr char FirmwareVersion[] = "3.2.0";
inline constexpr char DeviceName[] = "AegisCare ESP32-CAM";
inline constexpr char Hostname[] = "aegiscare-cam";

#ifndef AEGISCARE_WIFI_SSID
#define AEGISCARE_WIFI_SSID ""
#endif
#ifndef AEGISCARE_WIFI_PASSWORD
#define AEGISCARE_WIFI_PASSWORD ""
#endif

inline constexpr char StationSsid[] = AEGISCARE_WIFI_SSID;
inline constexpr char StationPassword[] = AEGISCARE_WIFI_PASSWORD;
inline constexpr char AccessPointSsid[] = "AegisCare-CAM";
inline constexpr char AccessPointPassword[] = "AegisCare2026";

inline constexpr uint16_t HttpPort = 80;
inline constexpr uint32_t SerialBaud = 115200;
inline constexpr uint32_t WifiConnectTimeoutMs = 15000;
inline constexpr uint32_t WifiHealthIntervalMs = 2000;
inline constexpr uint32_t WifiReconnectMaximumMs = 10000;

// AI-Thinker ESP32-CAM / OV2640 pin assignment.
inline constexpr int PinPwdn = 32;
inline constexpr int PinReset = -1;
inline constexpr int PinXclk = 0;
inline constexpr int PinSiod = 26;
inline constexpr int PinSioc = 27;
inline constexpr int PinY9 = 35;
inline constexpr int PinY8 = 34;
inline constexpr int PinY7 = 39;
inline constexpr int PinY6 = 36;
inline constexpr int PinY5 = 21;
inline constexpr int PinY4 = 19;
inline constexpr int PinY3 = 18;
inline constexpr int PinY2 = 5;
inline constexpr int PinVsync = 25;
inline constexpr int PinHref = 23;
inline constexpr int PinPclk = 22;
inline constexpr int PinFlashLed = 4;

inline constexpr framesize_t DefaultFrameSize = FRAMESIZE_QVGA;
inline constexpr int JpegQuality = 12;
inline constexpr int XclkFrequencyHz = 10000000;
inline constexpr uint32_t MinimumFrameIntervalMs = 65;

} // namespace AegisCare::Config
