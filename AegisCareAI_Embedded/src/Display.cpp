#include "Display.h"

namespace aegis {

Display::Display() : oled_(U8G2_R0, U8X8_PIN_NONE), ready_(false) {}

bool Display::begin() {
  oled_.setI2CAddress(static_cast<uint8_t>(config::kOledAddress << 1));
  oled_.begin();
  oled_.setFont(u8g2_font_6x10_tf);
  ready_ = true;
  return ready_;
}

void Display::showLogo() {
  if (!ready_) return;
  oled_.firstPage();
  do {
    oled_.drawRFrame(3, 3, 122, 58, 8);
    oled_.setFont(u8g2_font_9x15B_tf);
    oled_.drawStr(13, 28, "AegisCare");
    oled_.setFont(u8g2_font_6x10_tf);
    oled_.drawStr(42, 45, "AI  +");
  } while (oled_.nextPage());
}

void Display::showBootCheck(const __FlashStringHelper* label, bool passed) {
  if (!ready_) return;
  oled_.firstPage();
  do {
    drawHeader(F("SYSTEM CHECK"));
    oled_.setCursor(4, 31);
    oled_.print(label);
    oled_.setCursor(4, 49);
    oled_.print(passed ? F("[ OK ]") : F("[ FAIL ]"));
  } while (oled_.nextPage());
}

void Display::showProgress(uint8_t percent) {
  if (!ready_) return;
  oled_.firstPage();
  do {
    oled_.drawStr(39, 21, "LOADING");
    oled_.drawFrame(9, 34, 110, 12);
    const uint8_t width = static_cast<uint8_t>((percent * 106UL) / 100UL);
    oled_.drawBox(11, 36, width, 8);
  } while (oled_.nextPage());
}

void Display::showMenu(const __FlashStringHelper* const* labels, uint8_t itemCount,
                       uint8_t selectedIndex) {
  if (!ready_ || itemCount == 0) return;
  oled_.firstPage();
  do {
    drawHeader(F("MAIN MENU"));
    for (uint8_t row = 0; row < 3; ++row) {
      const uint8_t index = static_cast<uint8_t>((selectedIndex + row) % itemCount);
      const uint8_t y = static_cast<uint8_t>(25 + row * 15);
      if (row == 0) {
        oled_.drawRBox(1, y - 10, 126, 13, 3);
        oled_.setDrawColor(0);
      }
      oled_.setCursor(5, y);
      oled_.print(row == 0 ? F("> ") : F("  "));
      oled_.print(labels[index]);
      oled_.setDrawColor(1);
    }
  } while (oled_.nextPage());
}

void Display::showPage(const __FlashStringHelper* title,
                       const __FlashStringHelper* line1,
                       const __FlashStringHelper* line2,
                       const __FlashStringHelper* line3) {
  if (!ready_) return;
  const __FlashStringHelper* lines[] = {line1, line2, line3};
  oled_.firstPage();
  do {
    drawHeader(title);
    for (uint8_t index = 0; index < 3; ++index) {
      oled_.setCursor(3, 24 + index * 12);
      oled_.print(lines[index]);
    }
    oled_.drawStr(3, 63, "LEFT: Back");
  } while (oled_.nextPage());
}

void Display::showPatient(const char* name, const char* idSuffix, uint16_t position,
                          uint16_t total, bool highlighted) {
  if (!ready_) return;
  oled_.firstPage();
  do {
    oled_.drawStr(29, 9, "AegisCare AI");
    oled_.drawHLine(0, 11, 128);
    oled_.drawStr(1, 27, position > 1 ? "^" : " ");
    if (highlighted) {
      oled_.drawRBox(9, 14, 116, 18, 3);
      oled_.setDrawColor(0);
    }
    oled_.drawStr(13, 27, name);
    oled_.setDrawColor(1);
    oled_.drawStr(2, 43, "ID:");
    oled_.drawStr(24, 43, idSuffix);
    oled_.setCursor(2, 60);
    oled_.print(F("Patient "));
    oled_.print(position);
    oled_.print(F(" / "));
    oled_.print(total);
    oled_.drawStr(120, 60, position < total ? "v" : " ");
  } while (oled_.nextPage());
}

void Display::showScanMode(bool locked) {
  if (!ready_) return;
  oled_.firstPage();
  do {
    drawHeader(F("SCAN MODE"));
    oled_.drawStr(8, 27, locked ? "Target Locked" : "Move Target");
    oled_.drawStr(8, 42, "Joystick: Align");
    oled_.drawStr(8, 57, "B1 Stop  Click Lock");
  } while (oled_.nextPage());
}

void Display::showVoiceNotes(bool recording) {
  if (!ready_) return;
  oled_.firstPage();
  do {
    drawHeader(F("Clinical Notes"));
    oled_.drawStr(7, 30, recording ? "REC  Speak Now" : "Processing...");
    oled_.drawStr(7, 47, recording ? "B2 Stop" : "Please Wait");
    if (recording) oled_.drawDisc(116, 25, 4);
  } while (oled_.nextPage());
}

void Display::showReportReady() {
  if (!ready_) return;
  oled_.firstPage();
  do {
    drawHeader(F("Report Ready"));
    for (uint8_t y = 18; y < 48; y += 6) {
      for (uint8_t x = 8; x < 44; x += 6) {
        if (((x + y) / 6) % 2 == 0 || x < 14 || y < 24) oled_.drawBox(x, y, 4, 4);
      }
    }
    oled_.drawStr(57, 29, "QR");
    oled_.drawStr(57, 43, "Scan Phone");
    oled_.drawStr(57, 57, "Demo URL");
  } while (oled_.nextPage());
}

void Display::showEmergency() {
  if (!ready_) return;
  oled_.firstPage();
  do {
    oled_.drawBox(0, 0, 128, 15);
    oled_.setDrawColor(0);
    oled_.drawStr(32, 11, "EMERGENCY");
    oled_.setDrawColor(1);
    oled_.drawStr(8, 31, "Priority");
    oled_.setFont(u8g2_font_9x15B_tf);
    oled_.drawStr(8, 50, "CRITICAL");
    oled_.setFont(u8g2_font_6x10_tf);
    oled_.drawStr(8, 63, "Waiting Doctor");
  } while (oled_.nextPage());
}

void Display::showDeveloper() {
  if (!ready_) return;
  oled_.firstPage();
  do {
    oled_.drawRFrame(4, 5, 120, 54, 6);
    oled_.setFont(u8g2_font_9x15B_tf);
    oled_.drawStr(15, 28, "Soham Gosavi");
    oled_.setFont(u8g2_font_6x10_tf);
    oled_.drawStr(43, 45, "Developer");
    oled_.drawStr(30, 57, "AegisCare AI");
  } while (oled_.nextPage());
}

bool Display::isReady() const { return ready_; }

void Display::drawHeader(const __FlashStringHelper* title) {
  oled_.setFont(u8g2_font_6x10_tf);
  oled_.setDrawColor(1);
  oled_.setCursor(3, 9);
  oled_.print(title);
  oled_.drawHLine(0, 12, 128);
}

}  // namespace aegis
