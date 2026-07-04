#pragma once

#include <Arduino.h>

#include "Display.h"

namespace aegis {

enum class Page : uint8_t {
  Menu,
  Patient,
  Scan,
  DigitalTwin,
  Reports,
  Settings
};

/** Maps menu selections to pages and renders their concise operator guidance. */
class Pages {
 public:
  explicit Pages(Display& display);

  /** Opens the page corresponding to a main-menu index. */
  void open(uint8_t menuIndex);

  /** Returns to the main menu. */
  void close();

  /** Returns the active page. */
  Page current() const;

  /** Renders the active non-menu page. */
  void render(bool serialHealthy, bool inputHealthy);

 private:
  Display& display_;
  Page current_;
};

}  // namespace aegis
