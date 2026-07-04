# Phase 3.1 Workflow

AegisCare AI remains an educational demonstration system. It does not diagnose disease, detect pathology, or provide clinical decisions.

Core flow:

Patient → History → Digital Twin → Guided Screening → Clinical Voice Notes → Preventive Coordinator → Report → QR → Follow-up

Controller behavior:

- Arduino Nano is the primary workflow controller.
- Desktop is the visualization engine.
- Button 1 starts/stops the demonstration scan.
- Button 2 toggles clinical voice notes.
- Button 3 generates a report only after scan completion.
- Button 4 activates emergency mode at any time.

Every generated scan summary and report must include:

Educational Demonstration Prototype  
Not Intended For Clinical Diagnosis.
