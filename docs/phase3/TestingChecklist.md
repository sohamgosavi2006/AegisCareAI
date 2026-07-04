# Phase 3.1 Testing Checklist

Hardware:

- Button 1 starts scan.
- Button 1 stops scan.
- Joystick moves target only during scan mode.
- Joystick returns to menu navigation after scan stop.
- Button 2 starts/stops clinical notes.
- Button 3 blocks before scan completion.
- Button 3 generates report after scan completion.
- Button 4 activates emergency mode anytime.
- Joystick double click shows developer card.

Desktop:

- No crash on rapid button events.
- OLED and desktop remain synchronized.
- Voice note audio and transcript are saved.
- Report HTML is generated.
- Report database entry is created.
- Emergency event is saved.
- Disclaimer appears in scan/report workflows.

Verification performed:

- Desktop build succeeded.
- Desktop regression tests passed.
- Arduino Nano PlatformIO build succeeded.
