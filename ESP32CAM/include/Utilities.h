#pragma once

#include <Arduino.h>

namespace AegisCare::Utilities {
inline bool intervalElapsed(uint32_t now, uint32_t previous, uint32_t interval) {
    return static_cast<uint32_t>(now - previous) >= interval;
}
}

