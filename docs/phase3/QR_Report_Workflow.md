# Phase 3.1 QR and Report Workflow

Button 3 generates the patient report only after scan completion.

Report output:

- Local HTML file in `reports/`
- SQLite report record
- GitHub Pages-style URL
- QR/status packet to Arduino

The QR payload must contain only the public report URL. It must not contain patient details.

Report sections:

- Patient details
- Digital Twin summary
- Demonstration scan summary
- Clinical voice notes
- Preventive coordinator summary
- Timeline
- Educational prototype disclaimer

Required disclaimer:

Educational Demonstration Prototype  
Not Intended For Clinical Diagnosis.
