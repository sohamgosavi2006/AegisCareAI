#pragma once

#include <Arduino.h>

namespace aegis {

/** Maintains bounded selection state for the handheld workflow main menu. */
class Menu {
 public:
  static constexpr uint8_t kItemCount = 5;

  Menu();

  /** Selects the previous item with wrap-around. */
  void previous();

  /** Selects the next item with wrap-around. */
  void next();

  /** Returns the selected menu index. */
  uint8_t selectedIndex() const;

  /** Returns flash-resident menu labels. */
  static const __FlashStringHelper* const* labels();

 private:
  uint8_t selectedIndex_;
};

}  // namespace aegis
