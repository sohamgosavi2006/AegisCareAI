#pragma once

#include <U8g2lib.h>

#include "Config.h"

namespace aegis {

/** Provides all OLED drawing primitives and keeps hardware details isolated. */
class Display {
 public:
  Display();

  /** Initializes the SSD1306 controller and clears the panel. */
  bool begin();

  /** Draws the AegisCare product splash. */
  void showLogo();

  /** Draws one boot diagnostic and its result. */
  void showBootCheck(const __FlashStringHelper* label, bool passed);

  /** Draws a progress bar with a short loading label. */
  void showProgress(uint8_t percent);

  /** Draws a three-row scrolling menu. */
  void showMenu(const __FlashStringHelper* const* labels, uint8_t itemCount,
                uint8_t selectedIndex);

  /** Draws a titled page with up to three content lines. */
  void showPage(const __FlashStringHelper* title,
                const __FlashStringHelper* line1,
                const __FlashStringHelper* line2,
                const __FlashStringHelper* line3);

  /** Draws the synchronized patient selector without intermediate frames. */
  void showPatient(const char* name, const char* idSuffix, uint16_t position,
                   uint16_t total, bool highlighted);

  /** Draws scan-mode joystick guidance. */
  void showScanMode(bool locked);

  /** Draws the clinical voice-notes state. */
  void showVoiceNotes(bool recording);

  /** Draws report/QR status. */
  void showReportReady();

  /** Draws emergency state. */
  void showEmergency();

  /** Draws the developer double-click card. */
  void showDeveloper();

  /** Returns whether initialization succeeded. */
  bool isReady() const;

 private:
  void drawHeader(const __FlashStringHelper* title);

  U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled_;
  bool ready_;
};

}  // namespace aegis
