# AegisCare AI

AegisCare AI is an assisted rural-healthcare workflow demonstration built with C++20, Qt 6, and SQLite. It includes patient registration, a living digital health twin, simulated camera screening, multilingual operator guidance, anonymous community analytics, preventive follow-up coordination, and hardware emulation.

The application is not a medical device and does not diagnose disease. Camera feeds, screening visualizations, telemetry, and seeded records are demonstrations only.

## Run

This workspace contains a project-local Qt toolchain. From Terminal:

```sh
./run-aegiscare.sh
```

Demo credentials are `tech` / `tech` with the Technician role. Guest mode is also available.

## Verify

```sh
.venv/bin/ctest --test-dir build --output-on-failure
```

The smoke suite checks schema creation and migration, hashed authentication, patient and digital-twin creation, duplicate detection, atomic backup/restore, workflow transitions, and camera-quality bounds.

## Clean-machine setup

The project requires Qt 6 modules Core, Widgets, Charts, SQL, Network, SerialPort, and TextToSpeech. OpenCV is optional; when absent, a portable Qt-only camera-quality path is used. Configure with CMake 3.16+ and build with a C++20 compiler.
