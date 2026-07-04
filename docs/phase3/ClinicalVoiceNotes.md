# Phase 3.1 Clinical Voice Notes

Button 2 toggles clinical voice notes.

First press:

- Opens the clinical voice notes panel.
- Starts the recording workflow.
- Shows recording status and timer.
- Generates a live editable transcript draft.
- Shows recording state on the OLED.

Second press:

- Stops recording.
- Saves original WAV-compatible audio.
- Saves transcript.
- Creates a SQLite `voice_notes` record.
- Updates the digital twin timeline state.

Storage path:

`patient_data/<patient_id>/voice_notes/`

This feature records the doctor's observations only. It is not an AI chatbot and does not diagnose disease.
