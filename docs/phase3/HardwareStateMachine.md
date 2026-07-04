# Phase 3.1 Hardware State Machine

States:

- IDLE
- PATIENT_SELECTED
- DIGITAL_TWIN_READY
- SCAN_READY
- SCANNING
- SCAN_COMPLETED
- VOICE_RECORDING
- REPORT_READY
- EMERGENCY

Normal mode:

- Joystick Up/Down navigates OLED and desktop menus.
- Left returns/backtracks.
- Right advances to the next page/category.
- Click opens/selects.
- Double click shows the developer card for approximately three seconds.

Scan mode:

- Button 1 enters scan mode.
- Joystick menu navigation is disabled.
- Joystick moves only the scan target overlay.
- Click locks/unlocks the scan position.
- Button 1 exits scan mode and restores normal navigation.

Emergency mode:

- Button 4 is always available.
- Emergency does not delete recordings or reports.
