#include "Logger.h"
#include "Config.h"

namespace AegisCare {
void Logger::begin() {
    Serial.begin(Config::SerialBaud);
    delay(50);
    info(F("BOOT AegisCare ESP32-CAM"));
}

void Logger::info(const __FlashStringHelper* message) { write("INFO", String(message)); }
void Logger::info(const String& message) { write("INFO", message); }
void Logger::warning(const String& message) { write("WARN", message); }
void Logger::error(const String& message) { write("ERROR", message); }

void Logger::write(const char* level, const String& message) {
    Serial.printf("[%10lu] %-5s %s\n", millis(), level, message.c_str());
}
}

