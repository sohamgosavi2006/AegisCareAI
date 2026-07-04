#pragma once

#include <Arduino.h>

namespace aegis::config {

constexpr uint8_t kOledWidth = 128;
constexpr uint8_t kOledHeight = 64;
constexpr int8_t kOledResetPin = -1;
constexpr uint8_t kOledAddress = 0x3C;

constexpr uint8_t kJoystickXPin = A0;
constexpr uint8_t kJoystickYPin = A1;
constexpr uint8_t kJoystickSwitchPin = 2;
constexpr uint8_t kStartButtonPin = 3;
constexpr uint8_t kVoiceNotesButtonPin = 4;
constexpr uint8_t kReportButtonPin = 5;
constexpr uint8_t kEmergencyButtonPin = 6;
constexpr uint8_t kStatusLedPin = LED_BUILTIN;

constexpr unsigned long kSerialBaud = 115200;
constexpr unsigned long kDebounceMs = 35;
constexpr unsigned long kNavigationRepeatMs = 220;
constexpr unsigned long kDoubleClickMs = 360;
constexpr unsigned long kHeartbeatMs = 5000;
constexpr int kJoystickLowThreshold = 300;
constexpr int kJoystickHighThreshold = 700;
constexpr size_t kSerialBufferSize = 96;

}  // namespace aegis::config
