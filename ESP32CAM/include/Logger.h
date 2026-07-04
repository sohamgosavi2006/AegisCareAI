#pragma once

#include <Arduino.h>

namespace AegisCare {
class Logger final {
public:
    static void begin();
    static void info(const __FlashStringHelper* message);
    static void info(const String& message);
    static void warning(const String& message);
    static void error(const String& message);
private:
    static void write(const char* level, const String& message);
};
}

