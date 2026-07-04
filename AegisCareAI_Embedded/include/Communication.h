#pragma once

#include <Arduino.h>

#include "Config.h"
#include "PatientSync.h"

namespace aegis {

enum class DesktopCommand : uint8_t {
  None,
  StartScan,
  StopScan,
  PatientSelected,
  DigitalTwin,
  Settings,
  Workflow,
  ReportReady,
  QrReady,
  EmergencyMode,
  OledStatus,
  Ping,
  PatientList,
  PatientIndexSelected
};

/** Owns the newline-delimited JSON protocol over the Nano USB serial link. */
class Communication {
 public:
  explicit Communication(PatientSync& patientSync);

  /** Starts the serial transport. */
  void begin();

  /** Reads at most one complete command without blocking. */
  DesktopCommand poll();

  /** Announces that firmware initialization completed. */
  void sendDeviceReady();

  /** Sends a controller event to the desktop master. */
  void sendEvent(const __FlashStringHelper* eventName);

  /** Sends a device status response. */
  void sendStatus(const __FlashStringHelper* statusName);

  /** Immediately answers a desktop heartbeat. */
  void sendPong();

  /** Sends a typed joystick packet. */
  void sendJoystick(const __FlashStringHelper* direction);

  /** Sends a typed button packet. */
  void sendButton(const __FlashStringHelper* id);

  /** Sends scan-position joystick control while scan mode owns the joystick. */
  void sendScanControl(const __FlashStringHelper* action);

  /** Requests that the desktop select a patient index. */
  void sendPatientChanged(uint16_t index);

  /** Confirms the currently highlighted patient. */
  void sendPatientConfirm();

  /** Returns the index parsed from the latest patient_selected packet. */
  uint16_t selectedPatientIndex() const;

  /** Sends a periodic health message when due. */
  void heartbeat(unsigned long nowMs);

  /** Reports whether the USB serial interface is available. */
  bool selfTest() const;

 private:
  DesktopCommand parseCommand(const char* json) const;
  uint16_t parseIndex(const char* json) const;

  char buffer_[config::kSerialBufferSize];
  size_t length_;
  unsigned long lastHeartbeatAtMs_;
  PatientSync& patientSync_;
  uint16_t selectedPatientIndex_;
  bool streamingPatientList_;
  bool streamingPatientSelection_;
};

}  // namespace aegis
