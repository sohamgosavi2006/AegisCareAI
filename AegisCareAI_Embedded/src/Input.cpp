#include "Input.h"

#include "Config.h"
#include "Utilities.h"

namespace aegis {

Input::Input()
    : startButton_{config::kStartButtonPin, false, false, 0},
      voiceNotesButton_{config::kVoiceNotesButtonPin, false, false, 0},
      reportButton_{config::kReportButtonPin, false, false, 0},
      emergencyButton_{config::kEmergencyButtonPin, false, false, 0},
      joystickButton_{config::kJoystickSwitchPin, false, false, 0},
      lastNavigationAtMs_(0),
      lastJoystickClickAtMs_(0),
      pendingSingleClick_(false),
      lastDirection_(InputEvent::None) {}

void Input::begin() {
  pinMode(config::kJoystickSwitchPin, INPUT_PULLUP);
  pinMode(config::kStartButtonPin, INPUT_PULLUP);
  pinMode(config::kVoiceNotesButtonPin, INPUT_PULLUP);
  pinMode(config::kReportButtonPin, INPUT_PULLUP);
  pinMode(config::kEmergencyButtonPin, INPUT_PULLUP);
}

InputEvent Input::poll(unsigned long nowMs) {
  InputEvent event = pollButton(emergencyButton_, InputEvent::Emergency, nowMs);
  if (event != InputEvent::None) return event;
  event = pollButton(reportButton_, InputEvent::Report, nowMs);
  if (event != InputEvent::None) return event;
  event = pollButton(voiceNotesButton_, InputEvent::VoiceNotes, nowMs);
  if (event != InputEvent::None) return event;
  event = pollButton(startButton_, InputEvent::Start, nowMs);
  if (event != InputEvent::None) return event;
  event = pollJoystickButton(nowMs);
  return event != InputEvent::None ? event : pollJoystick(nowMs);
}

bool Input::selfTest() const {
  const int x = analogRead(config::kJoystickXPin);
  const int y = analogRead(config::kJoystickYPin);
  return x >= 0 && x <= 1023 && y >= 0 && y <= 1023;
}

InputEvent Input::pollButton(ButtonState& button, InputEvent event, unsigned long nowMs) {
  const bool pressed = digitalRead(button.pin) == LOW;
  if (pressed != button.sampledPressed) {
    button.sampledPressed = pressed;
    button.changedAtMs = nowMs;
  }
  if (pressed != button.stablePressed &&
      util::elapsed(nowMs, button.changedAtMs, config::kDebounceMs)) {
    button.stablePressed = pressed;
    if (pressed) return event;
  }
  return InputEvent::None;
}

InputEvent Input::pollJoystickButton(unsigned long nowMs) {
  const InputEvent click = pollButton(joystickButton_, InputEvent::Click, nowMs);
  if (click == InputEvent::Click) {
    if (pendingSingleClick_ && nowMs - lastJoystickClickAtMs_ <= config::kDoubleClickMs) {
      pendingSingleClick_ = false;
      lastJoystickClickAtMs_ = 0;
      return InputEvent::DoubleClick;
    }
    pendingSingleClick_ = true;
    lastJoystickClickAtMs_ = nowMs;
    return InputEvent::None;
  }
  if (pendingSingleClick_ && nowMs - lastJoystickClickAtMs_ > config::kDoubleClickMs) {
    pendingSingleClick_ = false;
    return InputEvent::Click;
  }
  return InputEvent::None;
}

InputEvent Input::pollJoystick(unsigned long nowMs) {
  const int x = analogRead(config::kJoystickXPin);
  const int y = analogRead(config::kJoystickYPin);
  InputEvent direction = InputEvent::None;
  if (y < config::kJoystickLowThreshold) direction = InputEvent::Up;
  else if (y > config::kJoystickHighThreshold) direction = InputEvent::Down;
  else if (x < config::kJoystickLowThreshold) direction = InputEvent::Left;
  else if (x > config::kJoystickHighThreshold) direction = InputEvent::Right;

  if (direction == InputEvent::None) {
    lastDirection_ = InputEvent::None;
    return InputEvent::None;
  }
  if (direction != lastDirection_ ||
      util::elapsed(nowMs, lastNavigationAtMs_, config::kNavigationRepeatMs)) {
    lastDirection_ = direction;
    lastNavigationAtMs_ = nowMs;
    return direction;
  }
  return InputEvent::None;
}

}  // namespace aegis
