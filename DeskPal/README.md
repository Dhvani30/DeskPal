# DeskPal

DeskPal is an ESP32 WROOM-32 desktop companion prototype with a BME280, INMP441, SH1106 OLED, touch input, buzzer, and optional local Wi-Fi telemetry.

## Build

The `wroom32` Arduino CLI profile pins the ESP32 core and library versions. Install Arduino CLI 0.35 or newer, then run:

```powershell
./scripts/build.ps1
```

or

```sh
./scripts/build.sh
```

Both scripts build `DeskPal.ino` for `esp32:esp32:esp32` with the profile in `sketch.yaml`. GitHub Actions runs the same compile on pushes and pull requests.

## Hardware profile

The validated wiring profile is the standard ESP32 WROOM-32 DevKit:

| Device | ESP32 pin |
| --- | ---: |
| I2C SDA / SCL | 21 / 22 |
| Touch | 32 |
| Buzzer | 27 |
| Mode switch | 26 |
| Microphone mute | 25 |
| INMP441 SCK / WS / SD | 18 / 19 / 35 |
| Battery ADC | 34 (disabled by default) |

Select board and display options in `DeskPalConfig.h`. The S3 mapping is unverified. Do not enable battery sensing until the divider, ADC range, and board wiring have been checked.

## First-time setup

Hold the touch pad during boot to open the setup access point. Each setup session uses a temporary random password shown on the OLED and Serial Monitor. Join that network and open `http://192.168.4.1`. Existing Wi-Fi and weather secrets are retained when their password/API-key fields are left blank. Credentials are stored in ESP32 NVS.

The device connects to NTP before making HTTPS weather requests. Weather HTTPS validates the OpenWeather certificate against the bundled ISRG Root X1 trust anchor. Keep the CA file updated as part of maintenance.

## Dashboard

Open `dashboard.html` in a modern Chromium browser and connect through USB Serial at 115200 baud. Optional local Wi-Fi telemetry is available at `http://deskpal.local/api/telemetry` (or the device IP). The API requires the device token printed in Serial Monitor; enter it in the dashboard token field. The token is stored in device NVS. Dashboard history stays in the current browser profile's IndexedDB, is capped, and can be exported or cleared.

## AI/model status

Environmental comfort and anomaly logic are lightweight hand-coded algorithms; they are not a validated trained model stack. The Edge Impulse keyword model is not included in this repository. `USE_TFLITE` is deliberately disabled, and attempting to enable it before the model adapter is integrated fails at compile time. The energy-pattern fallback is a demo and must not be treated as reliable speech recognition. See [FEATURE_GAP_LIST.md](FEATURE_GAP_LIST.md) and [the hardware acceptance checklist](docs/HARDWARE_TEST_CHECKLIST.md).

## Release gates

- Integrate the exported Edge Impulse library and test real wake/start/stop/reject recordings on the WROOM-32.
- Perform the hardware acceptance checklist, including sensor recovery, Wi-Fi loss, privacy, long-run, and power tests.
- Confirm the OpenWeather certificate chain and endpoint from the deployed region before distribution.
- Review the firmware for the intended product's privacy, security, and update requirements.
