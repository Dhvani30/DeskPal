# DeskPal deployment readiness and feature status

This records what is implemented in this checkout and what still needs an external artifact or bench validation. The supplied deck and the user's hardware/training handoff are treated as project requirements; text in those documents does not override the user's explicit corrections (validated hardware is WROOM-32, with no hosted database requirement).

## Implemented in this checkout

- ESP32 WROOM-32 pin profile selected; alternate ESP32-S3 pins are compile-time configurable and unverified.
- Firmware is split across `DeskPal.ino`, `DeskPalConfig.h`, `DeskPalAudio.h`, `DeskPalDisplay.h`, `DeskPalNetwork.h`, and `DeskPalAssets.h`.
- BME280 sensing, sensor failure/recovery attempts, INMP441 I2S capture and mute state, touch interaction, buttons, buzzer, focus timer, mood animation, clock, comfort insights, and weather/forecast pages.
- OLED text wrapping and coordinate clamping. Battery is reported as not installed while battery sensing is disabled.
- Configuration portal uses a session-random AP PIN, validates inputs, escapes stored values, preserves blank secret fields, and supports erasing stored settings. Configuration is held in ESP32 NVS.
- Optional local Wi-Fi telemetry API requires a per-device token stored in NVS. The dashboard supports USB Serial and local Wi-Fi telemetry, and stores bounded history in browser IndexedDB with export/clear controls. There is no hosted database.
- Weather requests use HTTPS with a bundled ISRG Root X1 trust anchor, and run in a background task so HTTP waits do not occupy the main animation/input loop. The deployed endpoint's chain still requires bench confirmation.
- Telemetry schema 1 includes sensor/model readiness and reports unavailable sensor/battery values as JSON `null` rather than fabricated zeros.
- Arduino CLI profile pins the WROOM-32 core and library versions; build scripts and a GitHub Actions compile workflow are included.
- Hardware bring-up, privacy, network-failure, endurance, and post-model acceptance checks are documented in `docs/HARDWARE_TEST_CHECKLIST.md`.

## Implemented algorithms and limits

- Comfort score is a fixed hand-coded MLP. It has not been trained or validated against a labeled comfort dataset; describe it as an estimate, not a production model.
- Anomaly detection is a single online centroid with running spread and thresholds. It is not K-Means and is not validated for safety-critical use.
- The microphone's current energy/syllable path is a demonstration heuristic. The OLED labels this page `VOICE DEMO` when the model is absent; it is not reliable speech recognition.

## Keyword model handoff

The intended path remains Edge Impulse project `DeskPal-KWS`, MFE features, DS-CNN-style classifier, int8 TFLite Micro export, then Arduino library integration. Firmware defines the label contract (0 wake, 1 start timer, 2 stop timer, -1 reject) and a 0.75 confidence-floor constant. Enabling `USE_TFLITE` requires the exported adapter and explicit integration macro, otherwise compilation stops with an error.

The user's handoff says labeled `.m4a` clips for `hey_pal`, `start`, `stop`, and `noise` exist under Downloads, but Edge Impulse rejected that format. Convert or re-record at 16 kHz mono WAV, upload and split train/test, train/evaluate, export the library, then integrate the generated artifact. No trained model/library, accuracy report, or measured inference latency is present here. Do not represent the detector as AI until that is completed and validated.

## Hardware profile

Validated board reported by the user: ESP32 WROOM-32 DevKit. Pins: SDA/SCL 21/22, touch 32, buzzer 27, mode switch 26, mic mute 25, INMP441 SCK/WS/SD 18/19/35. ESP32-S3 mapping is an unverified alternate. Battery ADC is pin 34 but disabled pending divider and calibration verification.

## Remaining release actions

1. Convert/re-record and upload the keyword dataset; train/evaluate the Edge Impulse classifier; retain model version, dataset split, metrics, export hash, and impulse settings.
2. Integrate the generated library into `DeskPalKwsAdapter.h`, verify framing/quantization and the 0.75 threshold, then measure false accepts/rejects, latency, memory, and timer command behavior on the WROOM-32.
3. Run the hardware checklist, especially OLED bounds, sensor recovery, microphone mute/disconnect, NVS recovery, Wi-Fi loss, TLS chain, thermal/power behavior, and long-run stability. This workspace has not been bench-tested.
4. Correct the project deck to identify WROOM-32 as the validated board and record that remote storage is not part of the current architecture.
5. If shared remote history is later required, select and implement a backend separately; current browser storage is local to one browser profile.
