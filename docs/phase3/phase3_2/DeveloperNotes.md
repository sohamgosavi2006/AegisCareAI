# Phase 3.2 Developer Notes

Phase 3.2 is an educational demonstration transport. Frames are displayed and measured for demonstration quality only; no frame is clinically interpreted and no diagnosis is generated.

## Performance decisions

- QVGA is the default to reduce current peaks, heap pressure and Wi-Fi bandwidth.
- VGA is available through an explicit endpoint.
- One or two frame buffers are selected according to PSRAM availability.
- `CAMERA_GRAB_LATEST` prevents stale-frame buildup.
- Stream rate is bounded and no frame-sized application allocation is introduced.
- Status traffic is small JSON on a two-second cadence.
- Desktop JPEG decoding remains on its dedicated worker thread.

## Extension rules

- Keep one `CameraManager`, one `StreamServer`, and one desktop `CameraStreamManager`.
- Add status fields compatibly; existing fields must retain their types.
- Never place credentials in committed source; use `Secrets.h`.
- Preserve the educational-prototype disclaimer in downstream scan/report UI.

## Future Phase 4

Phase 4 may add an SG90 positioning mechanism, HC-SR04 distance guidance and intelligent scan positioning. They are documentation-only future scope here and are not represented in Phase 3.2 firmware.

