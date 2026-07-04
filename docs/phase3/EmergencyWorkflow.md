# Phase 3.1 Emergency Workflow

Button 4 activates emergency response mode at any time.

Desktop actions:

- Switches to an emergency-oriented state.
- Records an emergency event in SQLite.
- Updates Digital Twin state.
- Announces emergency mode using the voice assistant.
- Shows a high-priority notification.

OLED actions:

- Displays Emergency Mode.
- Shows Priority Critical.

Database records:

- `emergency_events`
- `twin_states`
- `system_logs`

Emergency mode is a demonstration workflow. It is not medical triage and does not provide diagnosis.
