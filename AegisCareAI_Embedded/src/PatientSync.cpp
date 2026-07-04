#include "PatientSync.h"

#include <string.h>

namespace aegis {

PatientSync::PatientSync()
    : entries_{}, count_(0), selectedSlot_(0), total_(0), parsedNumber_(0),
      pendingIndex_(0), pendingId_{0}, pendingName_{0}, pendingNameLength_(0),
      window_{0}, windowLength_(0), capture_(Capture::None), escaped_(false) {}

void PatientSync::beginList() {
  count_ = 0;
  selectedSlot_ = 0;
  total_ = 0;
  parsedNumber_ = 0;
  pendingIndex_ = 0;
  pendingId_[0] = '\0';
  pendingName_[0] = '\0';
  pendingNameLength_ = 0;
  windowLength_ = 0;
  capture_ = Capture::None;
  escaped_ = false;
}

void PatientSync::consume(char value) {
  if (capture_ != Capture::None) {
    appendCapturedCharacter(value);
    return;
  }

  appendWindow(value);
  if (windowEndsWith("\"index\":")) beginCapture(Capture::Index);
  else if (windowEndsWith("\"total\":")) beginCapture(Capture::Total);
  else if (windowEndsWith("\"id\":\"")) beginCapture(Capture::Id);
  else if (windowEndsWith("\"name\":\"")) beginCapture(Capture::Name);
}

void PatientSync::finishList() {
  if (capture_ != Capture::None) finishCapture();
  if (total_ == 0) total_ = count_;
}

bool PatientSync::selectGlobal(uint16_t index) {
  for (uint8_t slot = 0; slot < count_; ++slot) {
    if (entries_[slot].index == index) {
      selectedSlot_ = slot;
      return true;
    }
  }
  return false;
}

bool PatientSync::selectPrevious() {
  if (selectedSlot_ == 0) return false;
  --selectedSlot_;
  return true;
}

bool PatientSync::selectNext() {
  if (selectedSlot_ + 1 >= count_) return false;
  ++selectedSlot_;
  return true;
}

uint16_t PatientSync::previousGlobalIndex() const {
  const uint16_t current = selectedGlobalIndex();
  return current == 0 ? 0 : current - 1;
}

uint16_t PatientSync::nextGlobalIndex() const {
  const uint16_t current = selectedGlobalIndex();
  return total_ == 0 || current + 1 >= total_ ? current : current + 1;
}

bool PatientSync::hasPatients() const { return count_ > 0; }
uint8_t PatientSync::count() const { return count_; }
uint16_t PatientSync::total() const { return total_; }
uint16_t PatientSync::selectedGlobalIndex() const {
  return count_ == 0 ? 0 : entries_[selectedSlot_].index;
}
uint16_t PatientSync::selectedPosition() const { return selectedGlobalIndex() + 1; }
const char* PatientSync::selectedName() const {
  return count_ == 0 ? "No patient" : entries_[selectedSlot_].name;
}
const char* PatientSync::selectedIdSuffix() const {
  return count_ == 0 ? "----" : entries_[selectedSlot_].idSuffix;
}

bool PatientSync::windowEndsWith(const char* token) const {
  const size_t tokenLength = strlen(token);
  return tokenLength <= windowLength_ &&
         memcmp(window_ + windowLength_ - tokenLength, token, tokenLength) == 0;
}

void PatientSync::appendWindow(char value) {
  if (windowLength_ < sizeof(window_)) {
    window_[windowLength_++] = value;
  } else {
    memmove(window_, window_ + 1, sizeof(window_) - 1);
    window_[sizeof(window_) - 1] = value;
  }
}

void PatientSync::beginCapture(Capture capture) {
  capture_ = capture;
  parsedNumber_ = 0;
  escaped_ = false;
  if (capture == Capture::Id) pendingId_[0] = '\0';
  if (capture == Capture::Name) {
    pendingNameLength_ = 0;
    pendingName_[0] = '\0';
  }
}

void PatientSync::finishCapture() {
  if (capture_ == Capture::Index) pendingIndex_ = parsedNumber_;
  else if (capture_ == Capture::Total) total_ = parsedNumber_;
  else if (capture_ == Capture::Name && count_ < kCapacity) {
    entries_[count_].index = pendingIndex_;
    strncpy(entries_[count_].name, pendingName_, kNameLength);
    strncpy(entries_[count_].idSuffix, pendingId_, kIdSuffixLength);
    ++count_;
  }
  capture_ = Capture::None;
  windowLength_ = 0;
}

void PatientSync::appendCapturedCharacter(char value) {
  if (capture_ == Capture::Index || capture_ == Capture::Total) {
    if (value >= '0' && value <= '9') {
      parsedNumber_ = static_cast<uint16_t>(parsedNumber_ * 10 + value - '0');
    } else {
      finishCapture();
      appendWindow(value);
    }
    return;
  }

  if (escaped_) {
    escaped_ = false;
    value = '?';
  } else if (value == '\\') {
    escaped_ = true;
    return;
  } else if (value == '"') {
    finishCapture();
    return;
  }

  if (capture_ == Capture::Name) {
    if (pendingNameLength_ < kNameLength - 1) {
      pendingName_[pendingNameLength_++] = value;
      pendingName_[pendingNameLength_] = '\0';
    }
  } else if (capture_ == Capture::Id) {
    const size_t length = strlen(pendingId_);
    if (length < kIdSuffixLength - 1) {
      pendingId_[length] = value;
      pendingId_[length + 1] = '\0';
    } else {
      memmove(pendingId_, pendingId_ + 1, kIdSuffixLength - 2);
      pendingId_[kIdSuffixLength - 2] = value;
      pendingId_[kIdSuffixLength - 1] = '\0';
    }
  }
}

}  // namespace aegis
