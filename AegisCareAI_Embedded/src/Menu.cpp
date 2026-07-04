#include "Menu.h"

namespace aegis {

Menu::Menu() : selectedIndex_(0) {}

void Menu::previous() {
  selectedIndex_ = selectedIndex_ == 0 ? kItemCount - 1 : selectedIndex_ - 1;
}

void Menu::next() { selectedIndex_ = static_cast<uint8_t>((selectedIndex_ + 1) % kItemCount); }

uint8_t Menu::selectedIndex() const { return selectedIndex_; }

const __FlashStringHelper* const* Menu::labels() {
  static const __FlashStringHelper* const labels[kItemCount] = {
      F("Patient Registry"), F("Guided Screening"), F("Digital Twin"),
      F("Reports"),          F("Settings")};
  return labels;
}

}  // namespace aegis
