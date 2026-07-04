#include "Utilities.h"

#include "Input.h"

namespace aegis::util {

bool elapsed(unsigned long nowMs, unsigned long sinceMs, unsigned long intervalMs) {
  return static_cast<unsigned long>(nowMs - sinceMs) >= intervalMs;
}

const __FlashStringHelper* inputEventName(uint8_t eventValue) {
  switch (static_cast<InputEvent>(eventValue)) {
    case InputEvent::Up: return F("UP");
    case InputEvent::Down: return F("DOWN");
    case InputEvent::Left: return F("LEFT");
    case InputEvent::Right: return F("RIGHT");
    case InputEvent::Click: return F("CLICK");
    case InputEvent::DoubleClick: return F("DOUBLE_CLICK");
    case InputEvent::Start: return F("START_SCAN");
    case InputEvent::VoiceNotes: return F("VOICE_NOTES");
    case InputEvent::Report: return F("REPORT");
    case InputEvent::Emergency: return F("EMERGENCY");
    case InputEvent::None: return F("NONE");
  }
  return F("NONE");
}

}  // namespace aegis::util
