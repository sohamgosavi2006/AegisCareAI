#pragma once

#include <Arduino.h>

namespace aegis::util {

/** Returns true when an interval has elapsed, including millis() rollover. */
bool elapsed(unsigned long nowMs, unsigned long sinceMs, unsigned long intervalMs);

/** Converts a navigation input to its protocol event name. */
const __FlashStringHelper* inputEventName(uint8_t eventValue);

}  // namespace aegis::util
