#pragma once

#include <Arduino.h>

namespace aegis {

/**
 * Stores a bounded desktop-provided patient window and incrementally parses the
 * patient_list JSON packet without allocating a second large serial buffer.
 */
class PatientSync {
 public:
  static constexpr uint8_t kCapacity = 4;
  static constexpr uint8_t kNameLength = 15;
  static constexpr uint8_t kIdSuffixLength = 5;

  struct Entry {
    uint16_t index;
    char name[kNameLength];
    char idSuffix[kIdSuffixLength];
  };

  PatientSync();

  void beginList();
  void consume(char value);
  void finishList();
  bool selectGlobal(uint16_t index);
  bool selectPrevious();
  bool selectNext();
  uint16_t previousGlobalIndex() const;
  uint16_t nextGlobalIndex() const;

  bool hasPatients() const;
  uint8_t count() const;
  uint16_t total() const;
  uint16_t selectedGlobalIndex() const;
  uint16_t selectedPosition() const;
  const char* selectedName() const;
  const char* selectedIdSuffix() const;

 private:
  enum class Capture : uint8_t { None, Index, Total, Id, Name };

  bool windowEndsWith(const char* token) const;
  void appendWindow(char value);
  void beginCapture(Capture capture);
  void finishCapture();
  void appendCapturedCharacter(char value);

  Entry entries_[kCapacity];
  uint8_t count_;
  uint8_t selectedSlot_;
  uint16_t total_;
  uint16_t parsedNumber_;
  uint16_t pendingIndex_;
  char pendingId_[kIdSuffixLength];
  char pendingName_[kNameLength];
  uint8_t pendingNameLength_;
  char window_[11];
  uint8_t windowLength_;
  Capture capture_;
  bool escaped_;
};

}  // namespace aegis
