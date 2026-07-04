#pragma once

#include "Display.h"

namespace aegis {

/** Coordinates the deterministic startup animation and hardware checks. */
class Animations {
 public:
  explicit Animations(Display& display);

  /** Shows the logo and loading sequence. */
  void playIntro();

  /** Displays a named self-test result. */
  void showCheck(const __FlashStringHelper* label, bool passed);

  /** Shows the final ready state. */
  void showReady();

 private:
  Display& display_;
};

}  // namespace aegis
