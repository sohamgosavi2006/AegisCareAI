#include <Arduino.h>

#include "Animations.h"
#include "Communication.h"
#include "Config.h"
#include "Display.h"
#include "Input.h"
#include "Menu.h"
#include "Pages.h"
#include "PatientSync.h"
#include "Utilities.h"

namespace aegis {

/** Composes the firmware modules and coordinates application-level behavior. */
class ControllerApplication {
 public:
  ControllerApplication()
      : communication_(patientSync_), animations_(display_), pages_(display_),
        inputHealthy_(false), dirty_(true), scanMode_(false), scanLocked_(false),
        voiceRecording_(false), temporaryScreenUntilMs_(0) {}

  /** Initializes hardware, runs diagnostics, and announces readiness. */
  void setup() {
    pinMode(config::kStatusLedPin, OUTPUT);
    digitalWrite(config::kStatusLedPin, LOW);
    communication_.begin();
    input_.begin();

    const bool displayHealthy = display_.begin();
    if (!displayHealthy) {
      signalFatalDisplayError();
      return;
    }

    animations_.playIntro();
    animations_.showCheck(F("OLED"), displayHealthy);
    inputHealthy_ = input_.selfTest();
    animations_.showCheck(F("JOYSTICK"), inputHealthy_);
    animations_.showCheck(F("BUTTONS"), true);
    animations_.showCheck(F("SERIAL"), communication_.selfTest());
    communication_.sendDeviceReady();
    animations_.showReady();
    render();
  }

  /** Processes transport and input events without blocking. */
  void loop() {
    const unsigned long nowMs = millis();
    handleDesktopCommand(communication_.poll());
    handleInput(input_.poll(nowMs));
    if (temporaryScreenUntilMs_ > 0 && nowMs >= temporaryScreenUntilMs_) {
      temporaryScreenUntilMs_ = 0;
      dirty_ = true;
    }
    if (dirty_) render();
  }

 private:
  /** Dispatches commands received from the desktop master. */
  void handleDesktopCommand(DesktopCommand command) {
    switch (command) {
      case DesktopCommand::StartScan:
        scanMode_ = true;
        scanLocked_ = false;
        pages_.open(1);
        communication_.sendStatus(F("SCAN_STARTED"));
        dirty_ = true;
        break;
      case DesktopCommand::StopScan:
        scanMode_ = false;
        scanLocked_ = false;
        communication_.sendStatus(F("SCAN_STOPPED"));
        dirty_ = true;
        break;
      case DesktopCommand::PatientSelected:
        pages_.open(0);
        dirty_ = true;
        break;
      case DesktopCommand::Ping:
        communication_.sendPong();
        break;
      case DesktopCommand::PatientList:
        if (pages_.current() == Page::Patient) dirty_ = true;
        break;
      case DesktopCommand::PatientIndexSelected:
        patientSync_.selectGlobal(communication_.selectedPatientIndex());
        pages_.open(0);
        dirty_ = true;
        break;
      case DesktopCommand::DigitalTwin:
        pages_.open(2);
        dirty_ = true;
        break;
      case DesktopCommand::Settings:
        pages_.open(4);
        dirty_ = true;
        break;
      case DesktopCommand::Workflow:
        pages_.open(3);
        dirty_ = true;
        break;
      case DesktopCommand::ReportReady:
      case DesktopCommand::QrReady:
        display_.showReportReady();
        temporaryScreenUntilMs_ = millis() + 5000UL;
        dirty_ = false;
        break;
      case DesktopCommand::EmergencyMode:
        display_.showEmergency();
        temporaryScreenUntilMs_ = millis() + 5000UL;
        dirty_ = false;
        break;
      case DesktopCommand::OledStatus:
        dirty_ = true;
        break;
      case DesktopCommand::None:
        break;
    }
  }

  /** Applies local controller actions and immediately forwards safety events. */
  void handleInput(InputEvent event) {
    if (event == InputEvent::None) return;
    if (event == InputEvent::Emergency) {
      digitalWrite(config::kStatusLedPin, HIGH);
      communication_.sendButton(F("EMERGENCY"));
      display_.showEmergency();
      temporaryScreenUntilMs_ = millis() + 5000UL;
      dirty_ = false;
      return;
    }
    if (event == InputEvent::Start) {
      scanMode_ = !scanMode_;
      scanLocked_ = false;
      communication_.sendButton(scanMode_ ? F("START") : F("STOP"));
      if (scanMode_) pages_.open(1);
      dirty_ = true;
      return;
    }
    if (event == InputEvent::VoiceNotes) {
      voiceRecording_ = !voiceRecording_;
      communication_.sendButton(voiceRecording_ ? F("VOICE_RECORD_START") : F("VOICE_RECORD_STOP"));
      display_.showVoiceNotes(voiceRecording_);
      temporaryScreenUntilMs_ = millis() + (voiceRecording_ ? 1500UL : 3500UL);
      dirty_ = false;
      return;
    }
    if (event == InputEvent::Report) {
      communication_.sendButton(F("REPORT"));
      display_.showPage(F("Report"), F("Generating..."), F("Desktop creates"), F("QR for phone"));
      temporaryScreenUntilMs_ = millis() + 3000UL;
      dirty_ = false;
      return;
    }
    if (event == InputEvent::DoubleClick) {
      communication_.sendEvent(F("ABOUT_DEVELOPER"));
      display_.showDeveloper();
      temporaryScreenUntilMs_ = millis() + 3000UL;
      dirty_ = false;
      return;
    }

    if (scanMode_) {
      handleScanInput(event);
      return;
    }

    if (pages_.current() == Page::Patient && patientSync_.hasPatients()) {
      handlePatientInput(event);
      return;
    }

    communication_.sendJoystick(util::inputEventName(static_cast<uint8_t>(event)));
    if (pages_.current() == Page::Menu) {
      if (event == InputEvent::Up) menu_.previous();
      else if (event == InputEvent::Down) menu_.next();
      else if (event == InputEvent::Click) pages_.open(menu_.selectedIndex());
      dirty_ = true;
    } else if (event == InputEvent::Left) {
      pages_.close();
      dirty_ = true;
    } else if (event == InputEvent::Click) {
      sendPageSelection();
    }
  }

  /** Keeps joystick menu navigation disabled while scan mode is active. */
  void handleScanInput(InputEvent event) {
    switch (event) {
      case InputEvent::Up:
        communication_.sendScanControl(F("UP"));
        break;
      case InputEvent::Down:
        communication_.sendScanControl(F("DOWN"));
        break;
      case InputEvent::Left:
        communication_.sendScanControl(F("LEFT"));
        break;
      case InputEvent::Right:
        communication_.sendScanControl(F("RIGHT"));
        break;
      case InputEvent::Click:
        scanLocked_ = !scanLocked_;
        communication_.sendScanControl(scanLocked_ ? F("LOCK") : F("UNLOCK"));
        break;
      default:
        return;
    }
    dirty_ = true;
  }

  /** Synchronizes patient navigation with the desktop registry. */
  void handlePatientInput(InputEvent event) {
    if (event == InputEvent::Up) {
      const uint16_t target = patientSync_.previousGlobalIndex();
      if (target != patientSync_.selectedGlobalIndex()) {
        patientSync_.selectPrevious();
        communication_.sendPatientChanged(target);
        dirty_ = true;
      }
    } else if (event == InputEvent::Down) {
      const uint16_t target = patientSync_.nextGlobalIndex();
      if (target != patientSync_.selectedGlobalIndex()) {
        patientSync_.selectNext();
        communication_.sendPatientChanged(target);
        dirty_ = true;
      }
    } else if (event == InputEvent::Click) {
      communication_.sendPatientConfirm();
      dirty_ = true;
    } else if (event == InputEvent::Left) {
      pages_.close();
      dirty_ = true;
    }
  }

  /** Sends the active page as a semantic desktop event. */
  void sendPageSelection() {
    switch (pages_.current()) {
      case Page::Patient: communication_.sendEvent(F("PATIENT_SELECTED")); break;
      case Page::Scan: communication_.sendEvent(F("START_SCAN")); break;
      case Page::DigitalTwin: communication_.sendEvent(F("DIGITAL_TWIN")); break;
      case Page::Reports: communication_.sendEvent(F("REPORTS")); break;
      case Page::Settings: communication_.sendEvent(F("SETTINGS")); break;
      case Page::Menu: break;
    }
  }

  /** Renders only when state has changed to minimize OLED flicker. */
  void render() {
    if (pages_.current() == Page::Menu) {
      display_.showMenu(Menu::labels(), Menu::kItemCount, menu_.selectedIndex());
    } else if (scanMode_) {
      display_.showScanMode(scanLocked_);
    } else if (pages_.current() == Page::Patient && patientSync_.hasPatients()) {
      display_.showPatient(patientSync_.selectedName(), patientSync_.selectedIdSuffix(),
                           patientSync_.selectedPosition(), patientSync_.total(), true);
    } else {
      pages_.render(communication_.selfTest(), inputHealthy_);
    }
    dirty_ = false;
  }

  /** Flashes the built-in LED when the OLED cannot initialize. */
  void signalFatalDisplayError() {
    for (uint8_t count = 0; count < 6; ++count) {
      digitalWrite(config::kStatusLedPin, !digitalRead(config::kStatusLedPin));
      delay(150);
    }
  }

  Display display_;
  Input input_;
  PatientSync patientSync_;
  Communication communication_;
  Menu menu_;
  Animations animations_;
  Pages pages_;
  bool inputHealthy_;
  bool dirty_;
  bool scanMode_;
  bool scanLocked_;
  bool voiceRecording_;
  unsigned long temporaryScreenUntilMs_;
};

/** Returns the single application instance without a global variable. */
ControllerApplication& application() {
  static ControllerApplication instance;
  return instance;
}

}  // namespace aegis

/** Arduino framework setup entry point. */
void setup() { aegis::application().setup(); }

/** Arduino framework event-loop entry point. */
void loop() { aegis::application().loop(); }
