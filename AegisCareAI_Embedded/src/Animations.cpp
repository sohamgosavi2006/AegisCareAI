#include "Animations.h"

#include <Arduino.h>

namespace aegis {

Animations::Animations(Display& display) : display_(display) {}

void Animations::playIntro() {
  display_.showLogo();
  delay(900);
  constexpr uint8_t kProgressSteps[] = {10, 35, 60, 85, 100};
  for (const uint8_t progress : kProgressSteps) {
    display_.showProgress(progress);
    delay(170);
  }
}

void Animations::showCheck(const __FlashStringHelper* label, bool passed) {
  display_.showBootCheck(label, passed);
  delay(330);
}

void Animations::showReady() {
  display_.showPage(F("DEVICE READY"), F("Desktop master"), F("USB serial active"), F("Opening menu..."));
  delay(700);
}

}  // namespace aegis
