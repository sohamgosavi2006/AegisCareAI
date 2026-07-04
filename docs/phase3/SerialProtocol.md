# Phase 3.1 Serial Protocol

Transport: newline-delimited JSON over USB serial.

Desktop and Arduino never exchange plain-text commands.

Key Arduino → Desktop packets:

```json
{"type":"button","id":"START"}
{"type":"button","id":"STOP"}
{"type":"button","id":"VOICE_RECORD_START"}
{"type":"button","id":"VOICE_RECORD_STOP"}
{"type":"button","id":"REPORT"}
{"type":"button","id":"EMERGENCY"}
{"type":"scan_control","action":"UP"}
{"type":"scan_control","action":"LOCK"}
{"event":"ABOUT_DEVELOPER"}
```

Key Desktop → Arduino packets:

```json
{"type":"patient_selected","id":"AEGIS-2026-0001","name":"Demo Patient"}
{"type":"oled_status","status":"SCAN_COMPLETED"}
{"type":"REPORT_READY","report_id":"AEGIS-0001"}
{"type":"qr_ready","url":"https://username.github.io/AegisCareAI/reports/AEGIS-0001.html"}
{"type":"EMERGENCY_MODE"}
```

Malformed JSON is ignored safely and must not trigger reconnects.
