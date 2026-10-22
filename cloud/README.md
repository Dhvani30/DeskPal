# DeskPal cloud memory

This optional service stores DeskPal readings and event messages in MongoDB Atlas. It also compares current room readings with the previous 24 hours and returns simple temperature and humidity suggestions. DeskPal still works normally if MongoDB is not configured or this service is offline.

## Use the existing weather key with the firmware

The firmware calls OpenWeather directly for its OLED weather and forecast pages. It cannot read a computer's `.env` file by itself. Before compiling in Arduino IDE, run this PowerShell command from the project root:

```powershell
powershell -ExecutionPolicy Bypass -File .\DeskPal\sync-weather-key.ps1
```

This reads `WEATHER_API_KEY` from the project-root `.env` and writes an ignored `DeskPal/DeskPalSecrets.h` file. The key is never printed or committed. The firmware uses this value for both current weather and forecast requests, overriding any older key saved in its setup page. The key is included in the firmware binary and can be extracted from a device image, as with any API key used directly on a microcontroller; restrict the key in your OpenWeather account if that option is available. If the `.env` value changes, run the command again before compiling and uploading.

The browser dashboard must be open and connected to DeskPal for new samples to upload. It sends one room sample per minute, device event messages, and comfort feedback votes when cloud memory is connected. It does not upload microphone audio. The service waits for at least 20 valid readings before making comparisons. Suggestions are informational; they do not change the ESP32 mood, timer, or hardware.

## Set up MongoDB Atlas step by step

You need a free MongoDB Atlas account, Node.js, and an internet connection. The root `.env` that contains the weather key is separate; MongoDB settings go in `cloud/.env`.

### 1. Create the database in Atlas

1. Sign in to MongoDB Atlas and create a project and a cluster (the free cluster is enough to start).
2. In **Database Access**, add a database user. Save its username and password; these are separate from your Atlas website login.
3. In **Network Access**, add the public IP address of the computer that will run the DeskPal cloud service. For a temporary test only, Atlas offers an allow-from-anywhere rule (`0.0.0.0/0`); it exposes database access more broadly, so remove it after testing.
4. In **Database** → **Connect** → **Drivers**, select Node.js and copy the connection string. Replace the placeholder username, password, and cluster address. If the password contains symbols, URL-encode them in the connection string.

### 2. Add the private cloud settings

Open a PowerShell terminal at the project root (the folder containing `dashboard.html`) and run:

```powershell
Copy-Item .\cloud\.env.example .\cloud\.env
notepad .\cloud\.env
```

In `cloud/.env`, replace these example values:

- `MONGODB_URI`: the Atlas connection string from step 1.
- `MONGODB_DB`: keep `deskpal`, or choose a database name.
- `DESKPAL_CLOUD_TOKEN`: replace the example with a private token of at least 24 randomly generated characters. You will enter the same token in the dashboard.
- `CORS_ORIGIN`: `*` is for local testing only. For a hosted dashboard, set this to the exact dashboard origin (or a comma-separated list of allowed origins).

Save the file. Do not paste its contents into chat or commit it; Git ignores `cloud/.env`.

### 3. Start the cloud service

In that same PowerShell terminal, run:

```powershell
Set-Location .\cloud
npm install
npm start
```

Keep this terminal open while using the dashboard. Visit `http://localhost:8787/health`; it should show `"mongoReady":true`. If it does not, check the Atlas IP allow-list, connection string, database username, and password.

### 4. Connect the dashboard

1. Open `dashboard.html` in Chrome or Edge, preferably through VS Code’s Live Server.
2. Connect the dashboard to DeskPal over Wi-Fi. For a direct connection, hold the touch pad while powering on, join the `DeskPal-xxxx` network shown on the OLED, and use `http://192.168.4.1`; copy the device token from the setup page. The direct network has no internet, so cloud uploads need router-connected mode instead.
3. In **Optional cloud memory and decisions**, enter `http://localhost:8787`, a device name such as `deskpal-01`, and the same `DESKPAL_CLOUD_TOKEN` from `cloud/.env`.
4. Choose **Connect cloud memory**. Leave the cloud service terminal running.

The dashboard sends one valid sensor sample per minute, device events, and each comfort vote to the cloud while connected. It sends no microphone audio. The first 20 valid samples (about 20 minutes) establish the room baseline. Suggestions are shown in the dashboard; they currently do not change the ESP32 mood or control hardware. Data is stored in the MongoDB collections `telemetry`, `events`, `decisions`, and `feedback`.

## Personal comfort learning

DeskPal learns on the ESP32 with two logistic thresholds per sensor: one for the lower edge of the comfortable band and one for its upper edge. Each vote trains the relevant threshold with stochastic gradient descent and a Gaussian prior that pulls estimates toward the selected climate zone's default range. Bounds keep the band in a plausible range. The displayed band is an estimate; it begins affecting comfort behavior after five votes for that sensor, and more feedback may improve it. It is a small adaptive model, not a calibrated or medical measurement.

The standalone learner check is in `tools/test_comfort_learner.cpp`. Its gradient-check row compares analytic derivatives with finite-difference estimates; matching values show the update direction was derived consistently. The vote table reports learned temperature bands after 5, 10, 20, and 40 simulated votes, plus the summed endpoint error from the hidden test range. At 40 votes, the test requires both endpoints to be within 1 C and the total error to improve on the default band. The final robustness line covers repeated identical votes, contradictory votes, NaN input, persistence round-tripping, and one-step undo. These deterministic simulated results check implementation behavior, not accuracy for every person's real preferences.

When cloud memory is connected, the dashboard posts each submitted vote to authenticated `POST /api/feedback`. The service validates the device ID, ISO timestamp, label, sensor kind, and finite sensor readings before storing them in `feedback`. These records preserve feedback history; the ESP32's local learner remains the source of its personalized ranges.

`localhost` works only when the dashboard and cloud service are on the same computer. For access from another device or a public deployment, host the service at a reachable HTTPS address and use that URL in the dashboard. Do not expose the prototype publicly with a shared token; add user authentication first.
