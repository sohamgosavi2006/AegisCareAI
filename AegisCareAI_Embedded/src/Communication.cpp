#include "Communication.h"

#include <string.h>

#include "Utilities.h"

namespace aegis {

Communication::Communication(PatientSync& patientSync)
    : buffer_{0}, length_(0), lastHeartbeatAtMs_(0), patientSync_(patientSync),
      selectedPatientIndex_(0), streamingPatientList_(false),
      streamingPatientSelection_(false) {}

void Communication::begin() { Serial.begin(config::kSerialBaud); }

DesktopCommand Communication::poll() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());
    if (incoming == '\n') {
      if (streamingPatientList_) {
        patientSync_.finishList();
        streamingPatientList_ = false;
        length_ = 0;
        return DesktopCommand::PatientList;
      }
      if (streamingPatientSelection_) {
        streamingPatientSelection_ = false;
        length_ = 0;
        return DesktopCommand::PatientIndexSelected;
      }
      buffer_[length_] = '\0';
      const DesktopCommand command = parseCommand(buffer_);
      if (command == DesktopCommand::PatientIndexSelected) {
        selectedPatientIndex_ = parseIndex(buffer_);
      }
      length_ = 0;
      return command;
    }
    if (streamingPatientList_) {
      patientSync_.consume(incoming);
      continue;
    }
    if (streamingPatientSelection_) continue;
    if (incoming != '\r' && length_ < config::kSerialBufferSize - 1) {
      buffer_[length_++] = incoming;
      buffer_[length_] = '\0';
      if (strstr(buffer_, "\"type\":\"patient_list\"") != nullptr) {
        patientSync_.beginList();
        for (size_t index = 0; index < length_; ++index) patientSync_.consume(buffer_[index]);
        streamingPatientList_ = true;
        length_ = 0;
      } else if (strstr(buffer_, "\"type\":\"patient_selected\"") != nullptr &&
                 strstr(buffer_, "\"index\":") != nullptr) {
        selectedPatientIndex_ = parseIndex(buffer_);
        streamingPatientSelection_ = true;
        length_ = 0;
      }
    } else if (length_ >= config::kSerialBufferSize - 1) {
      length_ = 0;
      // Malformed or oversized non-list packets are safely discarded.
    }
  }
  return DesktopCommand::None;
}

void Communication::sendDeviceReady() {
  Serial.println(F("{\"type\":\"device\",\"status\":\"DEVICE_READY\",\"device\":\"ARDUINO_NANO\",\"firmware\":\"2.1.0\"}"));
}

void Communication::sendEvent(const __FlashStringHelper* eventName) {
  Serial.print(F("{\"event\":\""));
  Serial.print(eventName);
  Serial.println(F("\"}"));
}

void Communication::sendStatus(const __FlashStringHelper* statusName) {
  Serial.print(F("{\"status\":\""));
  Serial.print(statusName);
  Serial.println(F("\"}"));
}

void Communication::sendPong() {
  Serial.println(F("{\"type\":\"heartbeat\",\"value\":\"PONG\"}"));
}

void Communication::sendJoystick(const __FlashStringHelper* direction) {
  Serial.print(F("{\"type\":\"joystick\",\"direction\":\""));
  Serial.print(direction);
  Serial.println(F("\"}"));
}

void Communication::sendButton(const __FlashStringHelper* id) {
  Serial.print(F("{\"type\":\"button\",\"id\":\""));
  Serial.print(id);
  Serial.println(F("\"}"));
}

void Communication::sendScanControl(const __FlashStringHelper* action) {
  Serial.print(F("{\"type\":\"scan_control\",\"action\":\""));
  Serial.print(action);
  Serial.println(F("\"}"));
}

void Communication::sendPatientChanged(uint16_t index) {
  Serial.print(F("{\"type\":\"patient_changed\",\"index\":"));
  Serial.print(index);
  Serial.println(F("}"));
}

void Communication::sendPatientConfirm() {
  Serial.println(F("{\"type\":\"patient_confirm\"}"));
}

uint16_t Communication::selectedPatientIndex() const { return selectedPatientIndex_; }

void Communication::heartbeat(unsigned long nowMs) {
  if (util::elapsed(nowMs, lastHeartbeatAtMs_, config::kHeartbeatMs)) {
    lastHeartbeatAtMs_ = nowMs;
    Serial.println(F("{\"status\":\"HEARTBEAT\",\"healthy\":true}"));
  }
}

bool Communication::selfTest() const { return true; }

DesktopCommand Communication::parseCommand(const char* json) const {
  if (strstr(json, "\"type\":\"heartbeat\"") != nullptr ||
      strstr(json, "\"type\":\"ping\"") != nullptr) return DesktopCommand::Ping;
  if (strstr(json, "\"type\":\"patient_selected\"") != nullptr)
    return DesktopCommand::PatientIndexSelected;
  if (strstr(json, "START_SCAN") != nullptr) return DesktopCommand::StartScan;
  if (strstr(json, "STOP_SCAN") != nullptr) return DesktopCommand::StopScan;
  if (strstr(json, "PATIENT_SELECTED") != nullptr) return DesktopCommand::PatientSelected;
  if (strstr(json, "DIGITAL_TWIN") != nullptr) return DesktopCommand::DigitalTwin;
  if (strstr(json, "REPORT_READY") != nullptr) return DesktopCommand::ReportReady;
  if (strstr(json, "QR_READY") != nullptr || strstr(json, "\"type\":\"qr_ready\"") != nullptr)
    return DesktopCommand::QrReady;
  if (strstr(json, "EMERGENCY_MODE") != nullptr) return DesktopCommand::EmergencyMode;
  if (strstr(json, "\"type\":\"oled_status\"") != nullptr) return DesktopCommand::OledStatus;
  if (strstr(json, "SETTINGS") != nullptr) return DesktopCommand::Settings;
  if (strstr(json, "WORKFLOW") != nullptr) return DesktopCommand::Workflow;
  return DesktopCommand::None;
}

uint16_t Communication::parseIndex(const char* json) const {
  const char* marker = strstr(json, "\"index\":");
  return marker == nullptr ? 0 : static_cast<uint16_t>(atoi(marker + 8));
}

}  // namespace aegis
