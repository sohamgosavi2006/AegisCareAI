#include "Pages.h"

namespace aegis {

Pages::Pages(Display& display) : display_(display), current_(Page::Menu) {}

void Pages::open(uint8_t menuIndex) {
  current_ = menuIndex < 5 ? static_cast<Page>(menuIndex + 1) : Page::Menu;
}

void Pages::close() { current_ = Page::Menu; }

Page Pages::current() const { return current_; }

void Pages::render(bool serialHealthy, bool inputHealthy) {
  switch (current_) {
    case Page::Patient:
      display_.showPage(F("PATIENT"), F("Registry synced"), F("UP/DOWN choose"), F("Click select"));
      break;
    case Page::Scan:
      display_.showPage(F("SCREENING"), F("B1 Start Scan"), F("ESP32 Demo"), F("No diagnosis"));
      break;
    case Page::DigitalTwin:
      display_.showPage(F("DIGITAL TWIN"), F("History + organs"), F("Demo timeline"), F("No diagnosis"));
      break;
    case Page::Reports:
      display_.showPage(F("REPORTS"), F("B3 Generate"), F("HTML + QR"), F("Demo only"));
      break;
    case Page::Settings:
      display_.showPage(F("SETTINGS"), serialHealthy ? F("Serial: READY") : F("Serial: ERROR"),
                        inputHealthy ? F("Inputs: READY") : F("Inputs: ERROR"), F("OLED: READY"));
      break;
    case Page::Menu:
      break;
  }
}

}  // namespace aegis
