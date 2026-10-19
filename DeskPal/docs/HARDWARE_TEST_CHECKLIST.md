# Hardware acceptance checklist

Record board revision, firmware version, library/core versions, ambient conditions, test time, and pass/fail evidence for every run. Run from a current-limited USB supply first; do not use a LiPo until the charging/protection path is verified.

## Bench bring-up

- [ ] Confirm WROOM-32 board identity and pin map against `DeskPalConfig.h`.
- [ ] Confirm OLED controller/address and verify every page stays inside 128x64 pixels.
- [ ] Confirm BME280 address, units, plausible temperature/humidity/pressure, and altitude assumptions.
- [ ] Disconnect the BME280 at boot and while running; verify bounded rediscovery, visible offline state, and recovery after reconnect.
- [ ] Confirm INMP441 channel/pin wiring, sample rate, clipping behavior, and microphone mute actually stops classification.
- [ ] Verify touch input, switch, button, buzzer, and timer transitions including held/repeated inputs.
- [ ] Leave battery sensing disabled until divider ratio, voltage limits, ADC calibration, and charge-state behavior are measured.

## Network and privacy

- [ ] Enter setup mode and confirm a new random AP password appears on the display; reject bad SSID/password/country/timezone/climate inputs.
- [ ] Verify blank secret fields preserve existing secrets and HTML values render safely for quotes and angle brackets.
- [ ] Verify config secrets remain in NVS after reboot and are not included in logs or dashboard telemetry.
- [ ] Verify telemetry requests without the per-device token receive HTTP 401; correct token works from the dashboard and an unknown origin cannot read data without the token.
- [ ] Confirm mDNS/IP connectivity, Wi-Fi loss/rejoin, NTP sync, TLS certificate validation, invalid API key, rate limit, and offline weather fallback.
- [ ] Inspect network traffic to ensure the weather key is sent only over HTTPS and never printed in serial logs.

## Long-run and fault recovery

- [ ] Run for 24 hours with periodic dashboard reads; record resets, watchdog events, minimum free heap, Wi-Fi retries, sensor failures, and timer drift.
- [ ] Power-cycle during configuration save and verify NVS remains usable and defaults recover cleanly from invalid stored data.
- [ ] Verify the timer and input/display behavior remain responsive during network failures.
- [ ] Verify OLED sleep/wake, quiet mode, microphone mute, and low-power behavior against measured current draw.
- [ ] Export and clear browser history; verify sample/event limits and behavior when IndexedDB is unavailable or full.

## Keyword model gate (after Edge Impulse export)

- [ ] Record model export hash/version and Edge Impulse impulse/sample-rate/window settings.
- [ ] Verify the 16 kHz input framing, channel conversion, MFE output, and tensor quantization against the generated library.
- [ ] Test wake/start/stop/reject with held-out speakers, distances, noise, silence, TV/music, muted mic, and microphone disconnect.
- [ ] Measure false accepts, false rejects, command cooldown behavior, peak inference latency, and free heap under repeated inference.
- [ ] Confirm command labels trigger each action once and reject/low-confidence outputs never alter the timer.
