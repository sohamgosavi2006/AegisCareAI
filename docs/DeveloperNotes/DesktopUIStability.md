# Desktop UI Stability and Responsive Redesign

## Scope

This update improves the existing Qt 6 interface without changing the project architecture or deleting historical documents. It focuses on crash prevention, safe repeated interaction, responsive layouts, visual consistency, and GUI-thread load reduction.

The application remains **Demonstration Mode** and an **Educational Prototype**. It is not a medical device and does not perform diagnosis or medical prediction.

## Critical crash fix

### Root cause

`DigitalTwinWidget` stored raw pointers to `QVariantAnimation` objects and started them with `QAbstractAnimation::DeleteWhenStopped`. Qt deleted an animation when it completed, but the corresponding member pointer remained non-null. A later call to `resetZoom()`, `zoomToRegion()`, or `triggerRotation()` dereferenced the stale pointer and could produce `EXC_BAD_ACCESS`.

### Resolution

- Rotation, zoom, and zoom-center animations are created once with `DigitalTwinWidget` ownership.
- Animations never self-delete.
- Repeated actions call `stop()`, update start/end values, and restart the same valid objects.
- Destruction explicitly stops animations and the glow timer before QObject child cleanup.
- A regression test waits beyond the former deletion time before invoking reset/rotation again.

## Safe interaction loop

- Login uses one parent-owned loading timer and a single-flight authentication flag.
- Login and Guest buttons disable during progress; repeated clicks cannot create extra timers or main windows.
- Navigation has a re-entry guard, preventing dashboard quick actions from refreshing a page twice.
- Patient search and duplicate checks use 180–220 ms debounce timers instead of querying SQLite on every keystroke.
- Patient, dashboard, analytics, preventive, digital-twin, settings, and screening actions use refresh/action guards where re-entry could duplicate work.
- Screening ignores duplicate starts, invalid frames, and inactive timer callbacks.
- Table access validates rows and items before dereferencing.
- Voice output stops the previous utterance/process before starting a new page explanation.
- Database updates are coalesced for 80 ms and refresh only the currently visible page.

## Responsive UI changes

- Every main view is hosted in a widget-resizable `QScrollArea`; controls remain reachable at smaller window sizes.
- Fixed camera and telemedicine canvases now use expanding size policies with sensible minimum sizes.
- Login is width-bounded rather than fixed-width and scales inside its layout.
- Global form controls have consistent minimum heights, focus states, disabled states, selection colors, padding, and tooltips.
- Dark medical styling remains the default; a persisted light high-contrast option is functional.
- Theme, language, voice, volume, and hardware address settings persist through `QSettings`.
- The top workflow bar carries a persistent demonstration/not-a-medical-device badge.
- Login adds connection readiness, workstation information, Enter-key login, fixed tab order, show-password, remembered operator ID/role, and a disabled password-reset explanation.

## Performance changes

- Hidden pages no longer rebuild charts and tables after every database write.
- Bursts of visit/twin database signals collapse into one visible-page refresh.
- Chart replacement uses deferred QObject deletion, avoiding immediate teardown during active UI events.
- Community analytics replaces repeated patient database lookups with an in-memory patient-to-village map.
- Camera frames remain responsive while deterministic quality scoring runs at 10 Hz instead of every 30 FPS frame.
- Patient search is debounced and serial synchronization remains event-driven.

These changes remove the largest avoidable GUI-thread spikes. File dialogs, SQLite backup/restore, and modal confirmations are intentionally synchronous operator actions.

## Functional cleanup

- “Cloud Sync” was renamed to “Workstation Settings”; it opens an actual settings page.
- Telemedicine demonstration notes now write to the local audit log rather than showing a false success message.
- Analytics guidance is explicitly rule-based demonstration guidance, not AI medical prediction.
- Screening completion and UI banners identify demonstration/educational status.
- Page navigation provides short, application-scoped voice guidance with offline Qt/native fallback.

## New files

- `DesktopApp/src/ui/UiTheme.h`
- `DesktopApp/src/ui/UiTheme.cpp`
- `DesktopApp/tests/UiInteractionTests.cpp`
- `DesktopApp/tests/LoginInteractionTests.cpp`
- `docs/DeveloperNotes/DesktopUIStability.md`

## Principal modified files

- `DesktopApp/src/ui/components/DigitalTwinWidget.cpp`
- `DesktopApp/src/ui/LoginWindow.h/.cpp`
- `DesktopApp/src/ui/MainWindow.h/.cpp`
- `DesktopApp/src/ui/PatientManagementView.h/.cpp`
- `DesktopApp/src/ui/DashboardView.h/.cpp`
- `DesktopApp/src/ui/AnalyticsView.h/.cpp`
- `DesktopApp/src/ui/PreventiveView.h/.cpp`
- `DesktopApp/src/ui/DigitalTwinView.h/.cpp`
- `DesktopApp/src/ui/ScreeningView.h/.cpp`
- `DesktopApp/src/ui/TelemedicineView.cpp`
- `DesktopApp/src/ui/SettingsView.h/.cpp`
- `DesktopApp/src/core/VoiceAssistant.h/.cpp`
- `DesktopApp/src/resources/styles/dark_theme.qss`
- `DesktopApp/src/main.cpp`
- `DesktopApp/CMakeLists.txt`

## Verification

Build:

```sh
cd DesktopApp
.venv/bin/cmake -S . -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$PWD/.venv/bin/ninja" \
  -DCMAKE_PREFIX_PATH="$PWD/third_party/Qt/6.8.3/macos" \
  -DCMAKE_BUILD_TYPE=Release
.venv/bin/cmake --build build --parallel
.venv/bin/ctest --test-dir build --output-on-failure
```

Verified tests:

1. `login_interaction_stability` — rapid operator/guest double-click emits one login each.
2. `ui_interaction_stability` — completed and rapid digital-twin animation interactions remain safe.
3. `communication_protocol` — partial/malformed/oversized JSON recovery remains intact.
4. `core_smoke` — database, workflow, camera-quality, and existing core behavior remain intact.

All four tests pass.

## Manual acceptance checklist

1. Resize login and main windows at multiple display scales; verify no unreachable controls.
2. Press Enter repeatedly during login; verify only one main window appears.
3. Rapidly alternate Digital Twin Rotate, organ click, and Reset Zoom for two minutes.
4. Switch all eight pages rapidly; confirm no repeated navigation, frozen page, or voice-process buildup.
5. Type quickly in patient search and confirm results update after the debounce without typing lag.
6. Select patients and confirm the table, details, QR, status bar, Digital Twin, screening, and Nano remain synchronized.
7. Start a demonstration scan and repeatedly click Start; verify one scan timer only.
8. Repeatedly refresh dashboard/community charts; verify no crash or persistent freeze.
9. Toggle dark/light theme and restart; verify persistence.
10. Save telemedicine demonstration notes and confirm a `TELEMEDICINE_DEMO` audit entry.

## Known limitations

- The UI interaction tests run using Qt’s offscreen platform; a final visual review on the target Mac display is still required for font rendering and OS-specific scaling.
- Database backup/restore and native file dialogs remain synchronous by design.
- Speech recognition and external Gemini/OpenRouter/Ollama integration were not added in this stability-focused update; speech synthesis and application-scoped page guidance remain offline-capable.
- ESP32-CAM and medical AI behavior were intentionally not expanded.

## Future roadmap

- Add screenshot-based layout regression baselines at 1024×768, 1440×900, and Retina scaling.
- Move large backup/restore copies to a worker with progress and cancellation.
- Add an optional local, constrained speech-command grammar for navigation.
- Introduce model/view table models if registry size grows beyond demonstration scale.

Suggested commit message:

```text
fix(ui): harden interaction lifetimes and improve responsive demo workflow
```
