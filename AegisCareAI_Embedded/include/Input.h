#pragma once

#include <Arduino.h>

namespace aegis {

enum class InputEvent : uint8_t {
  None,
  Up,
  Down,
  Left,
  Right,
  Click,
  DoubleClick,
  Start,
  VoiceNotes,
  Report,
  Emergency
};

/** Samples the joystick and buttons and emits debounced, repeat-limited events. */
class Input {
 public:
  Input();

  /** Configures all input pins. */
  void begin();

  /** Returns the highest-priority input event available for this loop. */
  InputEvent poll(unsigned long nowMs);

  /** Returns true when the joystick ADC values are within their valid range. */
  bool selfTest() const;

 private:
  struct ButtonState {
    uint8_t pin;
    bool stablePressed;
    bool sampledPressed;
    unsigned long changedAtMs;
  };

  InputEvent pollButton(ButtonState& button, InputEvent event, unsigned long nowMs);
  InputEvent pollJoystickButton(unsigned long nowMs);
  InputEvent pollJoystick(unsigned long nowMs);

  ButtonState startButton_;
  ButtonState voiceNotesButton_;
  ButtonState reportButton_;
  ButtonState emergencyButton_;
  ButtonState joystickButton_;
  unsigned long lastNavigationAtMs_;
  unsigned long lastJoystickClickAtMs_;
  bool pendingSingleClick_;
  InputEvent lastDirection_;
};

}  // namespace aegis
