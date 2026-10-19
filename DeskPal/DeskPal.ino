// ==================================================
// DeskPal - ESP32 desktop companion firmware
// ==================================================
// The keyword model is not bundled. The energy-based voice path is a demo.
// See README.md and docs/HARDWARE_TEST_CHECKLIST.md for setup and release gates.
// ==================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Preferences.h>
#include "time.h"
#include <math.h>
#include "driver/i2s.h"
#include <esp_task_wdt.h>

#include "DeskPalConfig.h"

// ==================================================
// 1. STRUCTS
// ==================================================
struct Eye {
  float x, y, w, h;
  float targetX, targetY, targetW, targetH;
  float pupilX, pupilY, targetPupilX, targetPupilY;
  float velX, velY, velW, velH, pVelX, pVelY;
  float k = 0.12f, d = 0.60f, pk = 0.08f, pd = 0.50f;
  bool blinking;
  unsigned long lastBlink, nextBlinkTime;
  void init(float _x, float _y, float _w, float _h) {
    x = targetX = _x; y = targetY = _y; w = targetW = _w; h = targetH = _h;
    pupilX = targetPupilX = 0; pupilY = targetPupilY = 0;
    nextBlinkTime = millis() + random(1000, 4000);
  }
  void update() {
    velX = (velX + (targetX - x) * k) * d; x += velX;
    velY = (velY + (targetY - y) * k) * d; y += velY;
    velW = (velW + (targetW - w) * k) * d; w += velW;
    velH = (velH + (targetH - h) * k) * d; h += velH;
    pVelX = (pVelX + (targetPupilX - pupilX) * pk) * pd; pupilX += pVelX;
    pVelY = (pVelY + (targetPupilY - pupilY) * pk) * pd; pupilY += pVelY;
  }
};

// Comfort zone expressed as center plus half-width derived from the
// per-region comfortable ranges in getClimateParams().
struct ClimateParams { float tMid, tHalf, hMid, hHalf; };
struct ForecastDay { char dayName[4]; int temp; char iconType[16]; };
struct BuzzerStep { uint16_t onMs; uint16_t offMs; };
#if ENABLE_WEATHER
struct WeatherSnapshot {
  bool ok;
  int weatherHttp, forecastHttp;
  float temperature, feelsLike;
  int humidity;
  char main[16], description[32];
  ForecastDay forecast[3];
};
#endif

// ==================================================
// 2. GLOBAL OBJECTS & VARIABLES
// ==================================================
#if USE_SH1106
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define OLED_WHITE SH110X_WHITE
#define OLED_BLACK SH110X_BLACK
#define DISPLAY_OFF() display.oled_command(SH110X_DISPLAYOFF)
#define DISPLAY_ON()  display.oled_command(SH110X_DISPLAYON)
#else
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define OLED_WHITE SSD1306_WHITE
#define OLED_BLACK SSD1306_BLACK
#define DISPLAY_OFF() display.ssd1306_command(SSD1306_DISPLAYOFF)
#define DISPLAY_ON()  display.ssd1306_command(SSD1306_DISPLAYON)
#endif

Adafruit_BME280 bme;
Preferences prefs;
#if ENABLE_WEATHER
WebServer configServer(80);
WeatherSnapshot weatherState = {};
portMUX_TYPE weatherStateMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t weatherTaskHandle = nullptr;
volatile bool weatherFetchInFlight = false;
#endif

Eye leftEye, rightEye;
bool micReady = false;
bool currentTm(struct tm* t);

#if ENABLE_WEATHER
// Browser dashboards may read the current device state over the local Wi-Fi
// network. The dashboard persists snapshots in its own IndexedDB database.
void sendTelemetryApi();
void handleCorsOptions();
void loadTelemetryToken();
WeatherSnapshot getWeatherSnapshot();
void publishWeatherSnapshot(const WeatherSnapshot& snapshot);
void getWeatherAndForecast();
void weatherFetchTask(void* parameter);
bool requestWeatherFetch();
#endif

int currentPage = 0;
bool highBrightness = true;
bool quietMode = false;
bool micMuted = false;
unsigned long lastPageSwitch = 0;

bool lastTouch = false;
unsigned long touchPressStart = 0;
unsigned long touchTransitions = 0;
unsigned long rubWindowStart = 0;
bool touchLongHoldFired = false;

bool lastBtn = false;
unsigned long btnPressStart = 0;
bool btnLongHandled = false;
bool btnVeryLongHandled = false;
bool btnHoldIsIncrement = false;
unsigned long lastBtnReleaseTime = 0;
unsigned long lastAutoIncrementTime = 0;

bool pendingSingleTap = false;
unsigned long pendingSingleTapTime = 0;
int pendingSingleTapAction = 0;

bool screenOff = false;
bool inConfigMode = false;
bool watchdogActive = false;

int currentMood = MOOD_NORMAL;
int manualMood = -1;
unsigned long manualMoodUntil = 0;
unsigned long kwsActiveUntil = 0;
unsigned long anomalyUntil = 0;
unsigned long lastInteraction = 0;
unsigned long pettedUntil = 0, annoyedUntil = 0, scaredUntil = 0, excitedUntil = 0;
int petCount = 0;
unsigned long petWindowStart = 0;
bool neglectSleepy = false;
float pupilScale = 1.0f;
float press2sAgo = NAN;
unsigned long press2sTime = 0, lastLiftCheck = 0, liftSuppressUntil = 0;

float envTemp = NAN, envHum = NAN, envPress = NAN, envAlt = NAN;
bool sensorsOk = false, bmpOk = false;
int sensorFailCount = 0;
unsigned long lastGoodSensorRead = 0;
int comfortClass = COMFORT_OK;

float centroid[3] = { 0, 0, 0 }, spread[3] = { 1, 1, 1 };
int kmSamples = 0;
bool anomalyActive = false;
char anomalyMsg[48] = "";
unsigned long anomalyBuzzedAt = 0;
float pressHourAgo = NAN;
unsigned long lastPressSample = 0;
float lastInferenceMs = 0.0f;
float infAvg = 0.0f;

char insights[6][80];
int insightCount = 0;
unsigned long lastSensorRead = 0;
unsigned long lastInsightGen = 0;

String city, countryCode, apiKey, wifiSsid, wifiPass, tzString;
char telemetryToken[33] = "";
char cityUpper[16] = "";
bool wifiOn = false;
bool wifiWant = false;
bool telemetryServerStarted = false;
unsigned long wifiRetryAt = 0;
volatile bool timeSynced = false;
unsigned long lastWeatherUpdate = 0;
const char* ntpServer = "pool.ntp.org";
const time_t demoBaseEpoch = 1767258000;

int climateZone = CLIMATE_TEMPERATE;
float batteryPct = NAN;
float micRms = 0.0f;
float micNoiseFloor = 200.0f;

float histTemp[60], histHum[60];
int histIdx = 0;
bool histFilled = false;

unsigned long timerDurationSec = TIMER_MIN_MINS * 60;
unsigned long timerRemainingSec = TIMER_MIN_MINS * 60;
bool timerRunning = false;
bool timerLock = false;
unsigned long timerLastTick = 0;
bool isTimerAlarm = false;

bool isListeningForCommand = false;
unsigned long listenStartTime = 0;
unsigned long lastKwsActionAt = 0;
unsigned long lastCmdSyllableAt = 0;
int commandSyllables = 0;
bool cmdInSyllable = false;

char aiResponse[96] = "";
unsigned long aiResponseStart = 0;
bool isAiResponding = false;

BuzzerStep buzzQueue[6];
int buzzQueueLen = 0;
int buzzQueueIdx = -1;
bool buzzPhaseOn = false;
unsigned long buzzPhaseStart = 0;

unsigned long lastSaccade = 0;
unsigned long saccadeInterval = 3000;
float breathVal = 0;

int32_t i2sBuf[256];
enum KwsState { KWS_IDLE, KWS_SYL1, KWS_GAP };
KwsState kwsState = KWS_IDLE;
unsigned long kwsSyl1Time = 0;
float kwsBars[16];

const char* MOOD_NAMES[MOOD_COUNT] = {
  "NORMAL", "HAPPY", "EXCITED", "LOVE", "ANGRY", "SCARED",
  "SURPRISED", "SLEEPY", "COLD", "SAD", "SUSPICIOUS"
};
const char* comfortNames[] = { "COMFORTABLE", "TOO HOT", "TOO COLD", "TOO HUMID", "TOO DRY" };


// ==================================================
// 3. FUNCTIONS
// ==================================================

int ci(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
float cf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Comfortable indoor ranges per region. Edit these to retune comfort.
// temperate: 18-26 C, 30-85 %
// coastal:   20-30 C, 40-90 %
// desert:    18-29 C, 15-65 %
// mountain:  14-24 C, 25-80 %
ClimateParams getClimateParams() {
  float tMin = 18.0f, tMax = 26.0f, hMin = 30.0f, hMax = 85.0f;
  if (climateZone == CLIMATE_COASTAL)      { tMin = 20.0f; tMax = 30.0f; hMin = 40.0f; hMax = 90.0f; }
  else if (climateZone == CLIMATE_DESERT)  { tMin = 18.0f; tMax = 29.0f; hMin = 15.0f; hMax = 65.0f; }
  else if (climateZone == CLIMATE_MOUNTAIN){ tMin = 14.0f; tMax = 24.0f; hMin = 25.0f; hMax = 80.0f; }
  ClimateParams z;
  z.tMid = (tMin + tMax) * 0.5f; z.tHalf = (tMax - tMin) * 0.5f;
  z.hMid = (hMin + hMax) * 0.5f; z.hHalf = (hMax - hMin) * 0.5f;
  return z;
}
const char* climateName() {
  if (climateZone == CLIMATE_COASTAL) return "coastal";
  if (climateZone == CLIMATE_DESERT) return "desert";
  if (climateZone == CLIMATE_MOUNTAIN) return "mountain";
  return "temperate";
}

void clampEyeTargets(Eye& e) {
  e.targetW = cf(e.targetW, 8.0f, 56.0f);
  e.targetH = cf(e.targetH, 6.0f, 48.0f);
  e.targetX = cf(e.targetX, 0.0f, (float)(SCREEN_WIDTH) - e.targetW);
  e.targetY = cf(e.targetY, 0.0f, (float)(SCREEN_HEIGHT) - e.targetH);
}
void clampEyeState(Eye& e) {
  e.w = cf(e.w, 4.0f, (float)SCREEN_WIDTH);
  e.h = cf(e.h, 4.0f, (float)SCREEN_HEIGHT);
  e.x = cf(e.x, 0.0f, (float)SCREEN_WIDTH - e.w);
  e.y = cf(e.y, 0.0f, (float)SCREEN_HEIGHT - e.h);
}
bool clipSegToRect(int& x0, int& y0, int& x1, int& y1, int rx, int ry, int rw, int rh) {
  x0 = ci(x0, rx, rx + rw - 1); x1 = ci(x1, rx, rx + rw - 1);
  y0 = ci(y0, ry, ry + rh - 1); y1 = ci(y1, ry, ry + rh - 1);
  x0 = ci(x0, 0, SCREEN_WIDTH - 1); x1 = ci(x1, 0, SCREEN_WIDTH - 1);
  y0 = ci(y0, 0, SCREEN_HEIGHT - 1); y1 = ci(y1, 0, SCREEN_HEIGHT - 1);
  return !(x0 == x1 && y0 == y1);
}
void drawClippedLine(int x0, int y0, int x1, int y1, int rx, int ry, int rw, int rh, uint16_t c) {
  if (clipSegToRect(x0, y0, x1, y1, rx, ry, rw, rh)) display.drawLine(x0, y0, x1, y1, c);
}
void drawClippedPixel(int x, int y, int rx, int ry, int rw, int rh, uint16_t c) {
  if (x >= rx && x < rx + rw && y >= ry && y < ry + rh && x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
    display.drawPixel(x, y, c);
}
void drawClippedFillCircle(int cx, int cy, int r, int rx, int ry, int rw, int rh, uint16_t c) {
  for (int dy = -r; dy <= r; dy++)
    for (int dx = -r; dx <= r; dx++)
      if (dx * dx + dy * dy <= r * r)
        drawClippedPixel(cx + dx, cy + dy, rx, ry, rw, rh, c);
}
void drawClippedBitmap(int bx, int by, const unsigned char* bmp, int bw, int bh, uint16_t c) {
  bx = ci(bx, 0, SCREEN_WIDTH - bw);
  by = ci(by, 0, SCREEN_HEIGHT - bh);
  display.drawBitmap(bx, by, bmp, bw, bh, c);
}
void drawFatArc(int cx, int cy, int r, int a0, int a1, int thick, int rx, int ry, int rw, int rh) {
  for (int a = a0; a <= a1; a += 3) {
    float rad = a * PI / 180.0f;
    int px = cx + (int)(cosf(rad) * r);
    int py = cy + (int)(sinf(rad) * r);
    drawClippedFillCircle(px, py, thick / 2, rx, ry, rw, rh, OLED_WHITE);
  }
}
void drawFatLine(int x0, int y0, int x1, int y1, int thick, int rx, int ry, int rw, int rh) {
  int dist = max(abs(x1 - x0), abs(y1 - y0));
  if (dist == 0) { drawClippedFillCircle(x0, y0, thick / 2, rx, ry, rw, rh, OLED_WHITE); return; }
  for (int s = 0; s <= dist; s += 2) {
    int px = x0 + (x1 - x0) * s / dist;
    int py = y0 + (y1 - y0) * s / dist;
    drawClippedFillCircle(px, py, thick / 2, rx, ry, rw, rh, OLED_WHITE);
  }
  drawClippedFillCircle(x1, y1, thick / 2, rx, ry, rw, rh, OLED_WHITE);
}

void refreshCityUpper() {
  size_t n = city.length();
  if (n > sizeof(cityUpper) - 1) n = sizeof(cityUpper) - 1;
  for (size_t i = 0; i < n; i++) cityUpper[i] = toupper(city[i]);
  cityUpper[n] = '\0';
}
void loadConfig() {
  if (!prefs.begin("deskpal", true)) {
    Serial.println("Preferences unavailable; using safe defaults");
    return;
  }
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  apiKey = prefs.getString("apikey", "");
  city = prefs.getString("city", "");
  countryCode = prefs.getString("country", "");
  tzString = prefs.getString("tz", "UTC0");
  climateZone = prefs.getInt("climate", CLIMATE_TEMPERATE);
  prefs.end();
  if (climateZone < CLIMATE_TEMPERATE || climateZone > CLIMATE_MOUNTAIN) {
    climateZone = CLIMATE_TEMPERATE;
    Serial.println("Invalid stored climate; reset to temperate");
  }
  refreshCityUpper();
}
bool saveConfig(const String& s, const String& p, const String& ak, const String& cty, const String& ctry, const String& tz, int cl) {
  if (!prefs.begin("deskpal", false)) return false;
  bool ok = prefs.putString("ssid", s) > 0 && prefs.putString("pass", p) > 0 &&
            prefs.putString("apikey", ak) > 0 && prefs.putString("city", cty) > 0 &&
            prefs.putString("country", ctry) > 0 && prefs.putString("tz", tz) > 0 &&
            prefs.putInt("climate", cl) > 0;
  prefs.end();
  return ok;
}
#if ENABLE_WEATHER
String htmlEscape(const String& value) {
  String escaped; escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    switch (value[i]) {
      case '&': escaped += "&amp;"; break;
      case '<': escaped += "&lt;"; break;
      case '>': escaped += "&gt;"; break;
      case '"': escaped += "&quot;"; break;
      case '\'': escaped += "&#39;"; break;
      default: escaped += value[i]; break;
    }
  }
  return escaped;
}

void handleConfigRoot() {
  prefs.begin("deskpal", true);
  String sS = htmlEscape(prefs.getString("ssid", ""));
  String sC = htmlEscape(prefs.getString("city", ""));
  String sT = htmlEscape(prefs.getString("country", ""));
  String sZ = htmlEscape(prefs.getString("tz", "UTC0"));
  int cl = prefs.getInt("climate", 0);
  prefs.end();
  String s0 = cl == 0 ? " selected" : "", s1 = cl == 1 ? " selected" : "";
  String s2 = cl == 2 ? " selected" : "", s3 = cl == 3 ? " selected" : "";
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DeskPal Config</title><style>
body{font-family:sans-serif;max-width:420px;margin:30px auto;padding:24px;background:#0c1929;color:#e8f4fc;}
h1{color:#5ba3f5;}input,select{width:100%;padding:10px;margin:6px 0;border:1px solid #2d4a6f;border-radius:6px;box-sizing:border-box;background:#1a2d47;color:#e8f4fc;}
button{width:100%;padding:12px;background:#3498db;color:#fff;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin-top:16px;}
label{display:block;margin-top:12px;color:#8ab4e8;font-size:14px;}</style></head><body>
<h1>DeskPal Setup</h1><form action="/save" method="POST">
<label>WiFi SSID</label><input name="ssid" value=")rawliteral" + sS + R"rawliteral(">
<label>WiFi Password</label><input name="pass" type="password">
<label>OpenWeather API Key (leave blank to keep saved key)</label><input name="apikey" type="password" autocomplete="new-password">
<label>City</label><input name="city" value=")rawliteral" + sC + R"rawliteral(">
<label>Country Code (2 letters, optional)</label><input name="country" value=")rawliteral" + sT + R"rawliteral(">
<label>Timezone (POSIX)</label><input name="tz" value=")rawliteral" + sZ + R"rawliteral(">
<label>Climate zone</label><select name="climate">
<option value="0")rawliteral" + s0 + R"rawliteral(>Temperate</option>
<option value="1")rawliteral" + s1 + R"rawliteral(>Coastal / Mumbai</option>
<option value="2")rawliteral" + s2 + R"rawliteral(>Desert / Rajasthan</option>
<option value="3")rawliteral" + s3 + R"rawliteral(>Mountain / Kashmir</option>
</select>
<button type="submit">Save &amp; Reboot</button></form>
<form action="/reset" method="POST" onsubmit="return confirm('Erase all saved Wi-Fi, weather, and telemetry-token settings?')">
<button type="submit">Erase saved settings</button></form></body></html>)rawliteral";
  configServer.send(200, "text/html", html);
}
void handleConfigSave() {
  String ssid = configServer.arg("ssid"), pass = configServer.arg("pass");
  String key = configServer.arg("apikey"), cty = configServer.arg("city");
  String country = configServer.arg("country"), tz = configServer.arg("tz");
  if (ssid.isEmpty() || ssid.length() > 32 || pass.length() > 63 || key.length() > 128 ||
      cty.length() > 64 || tz.length() > 64 || !(country.isEmpty() || country.length() == 2)) {
    configServer.send(400, "text/plain", "Invalid configuration field length or country code"); return;
  }
  for (size_t i = 0; i < country.length(); ++i) {
    if (!isalpha((unsigned char)country[i])) { configServer.send(400, "text/plain", "Country code must contain two letters"); return; }
    country[i] = toupper((unsigned char)country[i]);
  }
  for (size_t i = 0; i < tz.length(); ++i) {
    char ch = tz[i];
    if (!(isalnum((unsigned char)ch) || ch == '+' || ch == '-' || ch == ':' || ch == ',' || ch == '_' || ch == '.')) {
      configServer.send(400, "text/plain", "Timezone contains unsupported characters"); return;
    }
  }
  int cl = configServer.hasArg("climate") ? configServer.arg("climate").toInt() : CLIMATE_TEMPERATE;
  if (cl < CLIMATE_TEMPERATE || cl > CLIMATE_MOUNTAIN) { configServer.send(400, "text/plain", "Invalid climate zone"); return; }
  if (pass.isEmpty() || key.isEmpty()) {
    prefs.begin("deskpal", true);
    if (pass.isEmpty()) pass = prefs.getString("pass", "");
    if (key.isEmpty()) key = prefs.getString("apikey", "");
    prefs.end();
  }
  if (pass.length() > 0 && pass.length() < 8) { configServer.send(400, "text/plain", "Wi-Fi password must be empty or at least 8 characters"); return; }
  if (!saveConfig(ssid, pass, key, cty, country, tz.isEmpty() ? "UTC0" : tz, cl)) {
    configServer.send(500, "text/plain", "Could not save configuration"); return;
  }
  configServer.send(200, "text/html", "<html><body style='font-family:sans-serif;background:#0c1929;color:#e8f4fc;padding:40px;'><h2>Saved!</h2><p>Rebooting...</p></body></html>");
  delay(1500);
  ESP.restart();
}
void handleConfigReset() {
  if (!prefs.begin("deskpal", false)) { configServer.send(500, "text/plain", "Could not open settings"); return; }
  bool ok = prefs.clear(); prefs.end();
  if (!ok) { configServer.send(500, "text/plain", "Could not erase settings"); return; }
  configServer.send(200, "text/html", "<html><body><h2>Settings erased</h2><p>Rebooting...</p></body></html>");
  delay(500); ESP.restart();
}
void startConfigPortal() {
  inConfigMode = true;
  WiFi.mode(WIFI_AP);
  uint64_t mac = ESP.getEfuseMac();
  char ssid[24], pass[CONFIG_AP_PIN_LENGTH + 1];
  snprintf(ssid, sizeof(ssid), "DeskPal-%06llX", (unsigned long long)(mac & 0xFFFFFF));
  snprintf(pass, sizeof(pass), "%08lu", (unsigned long)(esp_random() % 100000000UL));
  if (!WiFi.softAP(ssid, pass)) Serial.println("Config AP start failed");
  Serial.print("Setup AP: "); Serial.print(ssid); Serial.print(" / password: "); Serial.println(pass);
  configServer.on("/", handleConfigRoot);
  configServer.on("/save", HTTP_POST, handleConfigSave);
  configServer.on("/reset", HTTP_POST, handleConfigReset);
  configServer.begin();
  display.clearDisplay(); display.setFont(NULL);
  display.setTextColor(OLED_WHITE); display.setCursor(0, 0);
  display.print("Config mode\n\nConnect to:\n");
  display.print(ssid);
  display.print("\nPIN: "); display.print(pass);
  display.print("\nOpen 192.168.4.1");
  display.display();
}
#endif

void queueBuzz(const BuzzerStep* steps, int n) {
  if (quietMode) return;
  if (n > 6) n = 6;
  for (int i = 0; i < n; i++) buzzQueue[i] = steps[i];
  buzzQueueLen = n;
  buzzQueueIdx = 0;
  buzzPhaseOn = true;
  buzzPhaseStart = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}
void updateBuzzer() {
  if (buzzQueueIdx < 0) return;
  unsigned long now = millis();
  BuzzerStep& s = buzzQueue[buzzQueueIdx];
  unsigned long elapsed = now - buzzPhaseStart;
  if (buzzPhaseOn) {
    if (elapsed >= s.onMs) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzPhaseOn = false;
      buzzPhaseStart = now;
      if (s.offMs == 0) {
        buzzQueueIdx++;
        if (buzzQueueIdx >= buzzQueueLen) buzzQueueIdx = -1;
        else { buzzPhaseOn = true; buzzPhaseStart = now; digitalWrite(BUZZER_PIN, HIGH); }
      }
    }
  } else {
    if (elapsed >= s.offMs) {
      buzzQueueIdx++;
      if (buzzQueueIdx >= buzzQueueLen) buzzQueueIdx = -1;
      else { buzzPhaseOn = true; buzzPhaseStart = now; digitalWrite(BUZZER_PIN, HIGH); }
    }
  }
}
void buzzTap()    { BuzzerStep s[] = {{25, 0}}; queueBuzz(s, 1); }
void buzzAlert()  { BuzzerStep s[] = {{90, 90}, {90, 0}}; queueBuzz(s, 2); }
void buzzKws()    { BuzzerStep s[] = {{60, 70}, {60, 70}, {140, 0}}; queueBuzz(s, 3); }
void buzzHappy()  { BuzzerStep s[] = {{40, 50}, {70, 0}}; queueBuzz(s, 2); }
void buzzGrumble(){ BuzzerStep s[] = {{150, 0}}; queueBuzz(s, 1); }
void buzzScared() { BuzzerStep s[] = {{30, 40}, {30, 40}, {30, 0}}; queueBuzz(s, 3); }
void buzzBoot()   { BuzzerStep s[] = {{60, 60}, {60, 60}, {120, 0}}; queueBuzz(s, 3); }

const float MLP_W1[4][3] = {
  {  2.0f,  0.0f, -0.3f }, { -2.0f,  0.0f, -0.3f },
  {  0.0f,  2.0f, -0.3f }, {  0.0f, -2.0f, -0.3f },
};
const float MLP_W2[5][4] = {
  { -0.5f, -0.5f, -0.5f, -0.5f }, {  1.0f, 0.0f, 0.0f, 0.0f },
  {  0.0f,  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, 1.0f, 0.0f },
  {  0.0f,  0.0f,  0.0f,  1.0f },
};
// Inputs are normalized against the region comfort range, so the MLP
// switching point lands on the range edges instead of a narrow band
// around a single midpoint.
int mlpComfortClassify(float T, float H) {
  ClimateParams z = getClimateParams();
  float x[2] = { (T - z.tMid) / z.tHalf * COMFORT_EDGE_SCALE,
                 (H - z.hMid) / z.hHalf * COMFORT_EDGE_SCALE };
  float h[4];
  for (int i = 0; i < 4; i++) h[i] = tanhf(x[0] * MLP_W1[i][0] + x[1] * MLP_W1[i][1] + MLP_W1[i][2]);
  int best = 0; float bestV = -1e9f;
  for (int o = 0; o < 5; o++) {
    float v = 0;
    for (int i = 0; i < 4; i++) v += MLP_W2[o][i] * h[i];
    if (v > bestV) { bestV = v; best = o; }
  }
  return best;
}
void updateOnlineAnomalyModel(float t, float h, float p) {
  float f[3] = { t, h, p };
  if (kmSamples == 0) { centroid[0] = t; centroid[1] = h; centroid[2] = p; }
  if (kmSamples < 30) {
    for (int i = 0; i < 3; i++) centroid[i] += 0.1f * (f[i] - centroid[i]);
    kmSamples++;
    return;
  }
  float dT = fabsf(f[0] - centroid[0]);
  float dH = fabsf(f[1] - centroid[1]);
  float dP = fabsf(f[2] - centroid[2]);
  anomalyMsg[0] = '\0';
  if (dT > spread[0] * 3.0f + 0.5f) strlcpy(anomalyMsg, "Anomaly: temperature spike!", sizeof(anomalyMsg));
  else if (dH > spread[1] * 3.0f + 2.0f) strlcpy(anomalyMsg, "Anomaly: humidity jump!", sizeof(anomalyMsg));
  else if (dP > spread[2] * 3.0f + 0.5f && millis() > liftSuppressUntil) strlcpy(anomalyMsg, "Anomaly: pressure shift!", sizeof(anomalyMsg));
  if (anomalyMsg[0] != '\0') {
    anomalyUntil = millis() + 10000;
    if (millis() - anomalyBuzzedAt > 12000) { buzzAlert(); anomalyBuzzedAt = millis(); }
    Serial.print("{\"type\":\"decision\",\"msg\":\""); Serial.print(anomalyMsg); Serial.println("\"}");
  }
  for (int i = 0; i < 3; i++) centroid[i] += 0.02f * (f[i] - centroid[i]);
  spread[0] += 0.05f * (dT - spread[0]);
  spread[1] += 0.05f * (dH - spread[1]);
  spread[2] += 0.05f * (dP - spread[2]);
}
bool isRelativelyCold() {
  if (isnan(envTemp)) return false;
  ClimateParams z = getClimateParams();
  return envTemp < (z.tMid - z.tHalf);
}
void addInsightOnce(const char* s) {
  for (int i = 0; i < insightCount; i++) if (strcmp(insights[i], s) == 0) return;
  if (insightCount < 6) strlcpy(insights[insightCount++], s, sizeof(insights[0]));
}
void generateInsights() {
  insightCount = 0;
  char buf[80];
  if (anomalyActive && anomalyMsg[0] != '\0') addInsightOnce(anomalyMsg);
  if (batteryPct < BATT_LOW_PCT) addInsightOnce("Battery low! Feed Pal some charge!");
  struct tm t; currentTm(&t);
  int hr = t.tm_hour;
  const char* tod = (hr >= 5 && hr < 12) ? "morning" : (hr < 17) ? "afternoon" : (hr < 21) ? "evening" : "night";
  if (!isnan(envTemp)) {
    ClimateParams z = getClimateParams();
    if (envTemp < z.tMid - z.tHalf) {
      snprintf(buf, sizeof(buf), "Brrr! Cold for a %s %s - Pal might catch a cold! Warm up?", climateName(), tod);
      addInsightOnce(buf);
    } else if (envTemp > z.tMid + z.tHalf) {
      snprintf(buf, sizeof(buf), "Hot %s! Hydrate or Pal gets grumpy!", tod);
      addInsightOnce(buf);
    } else {
      snprintf(buf, sizeof(buf), "Cozy %s temps - perfect for deep work!", tod);
      addInsightOnce(buf);
    }
  }
  if (!isnan(envHum)) {
    if (envHum > 88) addInsightOnce("Super muggy! Ventilate or the desk gets sticky!");
    else if (envHum < 25) addInsightOnce("Dry air alert! Sip water, human!");
    else {
      snprintf(buf, sizeof(buf), "Humidity %d%% - right in the comfy band!", (int)envHum);
      addInsightOnce(buf);
    }
  }
  if (!isnan(envPress) && !isnan(pressHourAgo)) {
    float dP = envPress - pressHourAgo;
    if (dP <= -1.5f) addInsightOnce("Pressure dropping - skies may sulk soon!");
    else if (dP >= 1.5f) addInsightOnce("Pressure rising - sunny vibes ahead!");
  }
#if ENABLE_WEATHER
  WeatherSnapshot weather = getWeatherSnapshot();
  if (wifiOn && weather.ok) {
    for (int i = 0; i < 3; i++) {
      if (weather.forecast[i].dayName[0] == '\0') continue;
      if (strcmp(weather.forecast[i].iconType, "Rain") == 0 || strcmp(weather.forecast[i].iconType, "Drizzle") == 0 || strcmp(weather.forecast[i].iconType, "Thunderstorm") == 0) {
        snprintf(buf, sizeof(buf), "%s looks rainy - umbrella buddy!", weather.forecast[i].dayName);
        addInsightOnce(buf);
        break;
      }
    }
    if (weather.forecast[0].dayName[0] != '\0') {
      if (weather.forecast[0].temp >= 35) { snprintf(buf, sizeof(buf), "Tomorrow %dC - keep cool, sip water!", weather.forecast[0].temp); addInsightOnce(buf); }
      else if (weather.forecast[0].temp <= 15) { snprintf(buf, sizeof(buf), "Tomorrow %dC - jacket weather!", weather.forecast[0].temp); addInsightOnce(buf); }
      else { snprintf(buf, sizeof(buf), "Tomorrow ~%dC - nice for a walk!", weather.forecast[0].temp); addInsightOnce(buf); }
    }
  }
#endif
  if (micRms > micNoiseFloor * 4.0f) addInsightOnce("Loud room! Pal covers ears... shhh!");
  if (insightCount == 0) addInsightOnce("All stable. Pal is happy at your desk!");
}
void runInference() {
  unsigned long t0 = micros();
  if (bmpOk && !isnan(envTemp) && !isnan(envHum)) {
    comfortClass = mlpComfortClassify(envTemp, envHum);
    updateOnlineAnomalyModel(envTemp, envHum, envPress);
  }
  lastInferenceMs = (micros() - t0) / 1000.0f;
  infAvg = infAvg * 0.8f + lastInferenceMs * 0.2f;
}

void i2cBusRecovery() {
  Serial.println("I2C: bus looks stuck - attempting recovery");
  Wire.end();
  pinMode(SCL_PIN, OUTPUT);
  pinMode(SDA_PIN, INPUT_PULLUP);
  for (int i = 0; i < 9; i++) {
    if (digitalRead(SDA_PIN) == HIGH) break;
    digitalWrite(SCL_PIN, HIGH); delayMicroseconds(5);
    digitalWrite(SCL_PIN, LOW);  delayMicroseconds(5);
  }
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW);  delayMicroseconds(5);
  digitalWrite(SCL_PIN, HIGH); delayMicroseconds(5);
  digitalWrite(SDA_PIN, HIGH); delayMicroseconds(5);
  Wire.begin(SDA_PIN, SCL_PIN);
  bmpOk = bme.begin() || bme.begin(0x76);
  Serial.println(bmpOk ? "I2C: BME280 recovered" : "I2C: BME280 still unresponsive after recovery");
}

void readSensors() {
  if (!bmpOk) {
    if (++sensorFailCount >= SENSOR_RECOVERY_FAILS) {
      sensorFailCount = 0;
      i2cBusRecovery();
    }
    if (millis() - lastGoodSensorRead > SENSOR_GRACE_MS) sensorsOk = false;
    return;
  }
  float tSum = 0, hSum = 0, pSum = 0;
  int validReadings = 0;
  for (int i = 0; i < 3; i++) {
    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0f;
    if (!isnan(t) && !isnan(h) && !isnan(p)) {
      tSum += t; hSum += h; pSum += p; validReadings++;
    }
    delay(50);
  }
  if (validReadings == 0) {
    sensorFailCount++;
    if (sensorFailCount >= SENSOR_RECOVERY_FAILS) { i2cBusRecovery(); sensorFailCount = 0; }
    if (millis() - lastGoodSensorRead > SENSOR_GRACE_MS) sensorsOk = false;
    return;
  }
  float t = (tSum / validReadings) + TEMP_OFFSET;
  float h = hSum / validReadings;
  float p = pSum / validReadings;
  if (!isnan(h)) h = cf(h, 0.0f, 100.0f);
  if (isnan(envTemp)) { envTemp = t; envHum = h; envPress = p; }
  else {
    if (fabsf(t - envTemp) < 5.0f) envTemp = envTemp * 0.7f + t * 0.3f;
    if (fabsf(h - envHum) < 20.0f) envHum = envHum * 0.7f + h * 0.3f;
    if (fabsf(p - envPress) < 10.0f) envPress = envPress * 0.7f + p * 0.3f;
  }
  sensorFailCount = 0;
  lastGoodSensorRead = millis();
  envAlt = bme.readAltitude(1013.25);
  sensorsOk = true;
  runInference();
  anomalyActive = (millis() < anomalyUntil);
  histTemp[histIdx] = envTemp;
  histHum[histIdx] = envHum;
  histIdx = (histIdx + 1) % 60;
  if (histIdx == 0) histFilled = true;
  if (millis() - lastPressSample > 60000) { pressHourAgo = envPress; lastPressSample = millis(); }
}
void liftCheck() {
  if (!bmpOk) return;
  unsigned long now = millis();
  if (now - lastLiftCheck < 250) return;
  lastLiftCheck = now;
  float p = bme.readPressure() / 100.0f;
  if (isnan(p)) return;
  if (isnan(press2sAgo)) { press2sAgo = p; press2sTime = now; return; }
  if (now - press2sTime >= 2000) {
    if (press2sAgo - p >= LIFT_THRESHOLD && now > scaredUntil + 5000) {
      scaredUntil = now + 4000; liftSuppressUntil = now + 8000; lastInteraction = now; buzzScared();
      Serial.println("{\"type\":\"decision\",\"msg\":\"Picked up - scared\"}");
    }
    press2sAgo = p; press2sTime = now;
  }
}
void computeMood() {
  unsigned long now = millis();
  neglectSleepy = (now - lastInteraction > NEGLECT_MS);
  int m;
  if (now < manualMoodUntil) m = manualMood;
  else if (now < scaredUntil) m = MOOD_SCARED;
  else if (now < annoyedUntil) m = MOOD_ANGRY;
  else if (now < kwsActiveUntil || now < excitedUntil) m = MOOD_EXCITED;
  else if (now < pettedUntil) m = MOOD_LOVE;
  else if (anomalyActive) m = MOOD_SURPRISED;
  else if (neglectSleepy || batteryPct < BATT_LOW_PCT) m = MOOD_SLEEPY;
  else if (isRelativelyCold()) m = MOOD_COLD;
  else {
    m = MOOD_NORMAL;
    switch (comfortClass) {
      case COMFORT_OK: m = MOOD_HAPPY; break;
      case COMFORT_HOT: m = MOOD_ANGRY; break;
      case COMFORT_HUMID: m = MOOD_SAD; break;
      case COMFORT_DRY: m = MOOD_SUSPICIOUS; break;
      case COMFORT_COLD: m = MOOD_COLD; break;
    }
#if ENABLE_WEATHER
    WeatherSnapshot weather = getWeatherSnapshot();
    if (wifiOn && weather.ok) {
      if (strcmp(weather.main, "Clear") == 0) m = MOOD_HAPPY;
      else if (strcmp(weather.main, "Rain") == 0 || strcmp(weather.main, "Drizzle") == 0) m = MOOD_SAD;
      else if (strcmp(weather.main, "Thunderstorm") == 0) m = MOOD_SURPRISED;
    }
#endif
  }
  currentMood = m;
  switch (m) {
    case MOOD_SCARED: pupilScale = 0.5f; break;
    case MOOD_LOVE: pupilScale = 1.25f; break;
    case MOOD_SURPRISED: pupilScale = 0.8f; break;
    default: pupilScale = 1.0f;
  }
}

#include "DeskPalAudio.h"

float readBatteryPct() {
#if ENABLE_BATTERY
  int raw = analogRead(BATT_PIN);
  float v = (raw / 4095.0f) * 3.3f * 2.0f;
  return cf((v - 3.0f) / 1.2f * 100.0f, 0.0f, 100.0f);
#else
  return NAN;
#endif
}
void emitDashboardTelemetry() {
  static unsigned long lastEmit = 0;
  if (millis() - lastEmit < 1000) return;
  lastEmit = millis(); batteryPct = readBatteryPct();
  Serial.print("{\"type\":\"telemetry\",\"schema\":1,\"uptimeMs\":"); Serial.print(millis());
  Serial.print(",\"sensorsOk\":"); Serial.print(sensorsOk ? "true" : "false");
  Serial.print(",\"sensorAgeMs\":"); if (lastGoodSensorRead) Serial.print(millis() - lastGoodSensorRead); else Serial.print("null");
  Serial.print(",\"micReady\":"); Serial.print(micReady ? "true" : "false");
  Serial.print(",\"modelReady\":"); Serial.print(KWS_MODEL_INTEGRATED ? "true" : "false");
  Serial.print(",\"t\":"); if (!sensorsOk || isnan(envTemp)) Serial.print("null"); else Serial.print(envTemp, 1);
  Serial.print(",\"h\":"); if (!sensorsOk || isnan(envHum)) Serial.print("null"); else Serial.print((int)envHum);
  Serial.print(",\"p\":"); if (!sensorsOk || isnan(envPress)) Serial.print("null"); else Serial.print((int)envPress);
  Serial.print(",\"mood\":\""); Serial.print(MOOD_NAMES[currentMood]);
  Serial.print("\",\"comfort\":\""); Serial.print(comfortNames[comfortClass]);
  Serial.print("\",\"infMs\":"); Serial.print(infAvg, 2);
  Serial.print(",\"batt\":"); if (isnan(batteryPct)) Serial.print("null"); else Serial.print(batteryPct, 0);
  Serial.print(",\"micMute\":"); Serial.print(micMuted ? "true" : "false");
  Serial.print(",\"micRms\":"); Serial.print(micRms, 0);
  Serial.print(",\"anomaly\":"); Serial.print(anomalyActive ? "true" : "false");
  Serial.print(",\"climate\":"); Serial.print(climateZone);
  Serial.print(",\"histT\":[");
  int n = histFilled ? 60 : histIdx;
  for (int i = 0; i < n; i++) { int idx = histFilled ? (histIdx + i) % 60 : i; Serial.print(histTemp[idx], 1); if (i < n - 1) Serial.print(","); }
  Serial.println("]}");
}


int maxMainPage() { return PAGE_TIMER; }
void recordTouchTransition(unsigned long now) {
  if (rubWindowStart == 0 || now - rubWindowStart > RUB_WINDOW_MS) { rubWindowStart = now; touchTransitions = 0; }
  touchTransitions++;
}
bool wakeScreenIfOff(unsigned long now) {
  if (!screenOff) return false;
  DISPLAY_ON(); screenOff = false; lastInteraction = now; return true;
}
void handleTouchPetting() {
  unsigned long now = millis();
  bool t = (digitalRead(TOUCH_PIN) == HIGH);
  if (t && !lastTouch) {
    if (wakeScreenIfOff(now)) { lastTouch = t; return; }
    recordTouchTransition(now); touchPressStart = now; touchLongHoldFired = false;
  } else if (t && lastTouch) {
    if (!touchLongHoldFired && now - touchPressStart >= TOUCH_LONG_HOLD_MS) { longPetAction(now); touchLongHoldFired = true; }
  } else if (!t && lastTouch) {
    if (!touchLongHoldFired) {
      unsigned long heldMs = now - touchPressStart;
      bool strongRub = (touchTransitions >= RUB_MIN_TRANSITIONS);
      if (heldMs < TOUCH_LONG_HOLD_MS) { petAction(now, strongRub); }
    }
    touchTransitions = 0; rubWindowStart = 0;
  }
  lastTouch = t;
}

void handleButtonNav() {
  unsigned long now = millis();
  bool b = (digitalRead(BTN_MICMUTE_PIN) == LOW);

   if (pendingSingleTap && (now - pendingSingleTapTime >= DOUBLE_TAP_DELAY_MS)) {
    pendingSingleTap = false;
    lastInteraction = now;
    if (pendingSingleTapAction == 2) {
      currentPage++;
      if (currentPage > maxMainPage()) currentPage = 0;
      lastSaccade = 0;
      buzzTap();
    } else if (pendingSingleTapAction == 3) {
      currentPage = PAGE_EYES;
      lastSaccade = 0;
      buzzTap();
    }
  }

  if (b && !lastBtn) {
    if (wakeScreenIfOff(now)) { lastBtn = b; return; }
    btnPressStart = now;
    btnLongHandled = false;
    btnVeryLongHandled = false;
    btnHoldIsIncrement = false;
  }
  else if (b && lastBtn) {
    unsigned long heldMs = now - btnPressStart;
    if (isTimerAlarm) {
      isTimerAlarm = false;
      timerDurationSec = TIMER_MIN_MINS * 60;
      timerRemainingSec = timerDurationSec;
      digitalWrite(BUZZER_PIN, LOW);
      buzzQueueIdx = -1;
      buzzTap();
      btnLongHandled = true;
      lastBtn = b;
      pendingSingleTap = false;
      return;
    }
        if (!btnVeryLongHandled && heldMs >= BTN_VERY_LONG_PRESS_MS) {
      btnVeryLongHandled = true;
      btnLongHandled = true;
      btnHoldIsIncrement = false;
      pendingSingleTap = false;
      lastInteraction = now;
      if (currentPage == PAGE_TIMER) {
        currentPage = PAGE_EYES;
        lastSaccade = 0;
        buzzTap();
      } else {
        highBrightness = !highBrightness;
#if USE_SH1106
        display.setContrast(highBrightness ? 255 : 1);
#else
        display.dim(!highBrightness);
#endif
        buzzTap();
      }
    }
    if (!btnLongHandled && heldMs >= BTN_LONG_PRESS_MS) {
      btnLongHandled = true;
      lastAutoIncrementTime = now;
      lastInteraction = now;
      pendingSingleTap = false;
      if (currentPage == PAGE_TIMER && !timerRunning) {
        btnHoldIsIncrement = true;
        timerDurationSec += 5 * 60;
        if (timerDurationSec > 120 * 60) timerDurationSec = 120 * 60;
        timerRemainingSec = timerDurationSec;
        buzzTap();
      } else {
        btnHoldIsIncrement = false;
        if (timerRunning) { timerRunning = false; timerLock = false; timerDurationSec = TIMER_MIN_MINS * 60; timerRemainingSec = timerDurationSec; buzzTap(); }
        else if (currentPage != PAGE_EYES) { currentPage = PAGE_EYES; lastSaccade = 0; buzzTap(); }
        else { micMuted = !micMuted; Serial.println(String("{\"type\":\"decision\",\"msg\":\"Mic ") + (micMuted ? "muted" : "unmuted") + "\"}"); buzzTap(); }
      }
    }
    if (btnHoldIsIncrement && !btnVeryLongHandled && heldMs < 3500 && currentPage == PAGE_TIMER && !isTimerAlarm) {
      if (now - lastAutoIncrementTime >= 500) {
        timerDurationSec += 5 * 60;
        if (timerDurationSec > 120 * 60) timerDurationSec = 120 * 60;
        timerRemainingSec = timerDurationSec;
        lastAutoIncrementTime = now;
        buzzTap();
      }
    }
  }
  else if (!b && lastBtn) {
    unsigned long heldMs = now - btnPressStart;
    lastAutoIncrementTime = 0;
    if (isTimerAlarm) {
      isTimerAlarm = false;
      timerDurationSec = TIMER_MIN_MINS * 60;
      timerRemainingSec = timerDurationSec;
      digitalWrite(BUZZER_PIN, LOW);
      buzzQueueIdx = -1;
      buzzTap();
      pendingSingleTap = false;
    }
    else if (!btnLongHandled && heldMs < BTN_LONG_PRESS_MS) {
      if (now - lastBtnReleaseTime < DOUBLE_TAP_DELAY_MS) {
        lastBtnReleaseTime = 0;
        pendingSingleTap = false;
        if (currentPage == PAGE_TIMER) {
          if (!timerRunning) { timerRunning = true; timerLastTick = now; buzzKws(); }
          else { timerRunning = false; timerDurationSec = TIMER_MIN_MINS * 60; timerRemainingSec = timerDurationSec; buzzTap(); }
        } else {
          currentPage++;
          if (currentPage > maxMainPage()) currentPage = 0;
          lastSaccade = 0;
          buzzTap();
        }
      } else {
        lastBtnReleaseTime = now;
        pendingSingleTap = true;
        pendingSingleTapTime = now;
        pendingSingleTapAction = (currentPage == PAGE_TIMER) ? 3 : 2;
      }
    }
  }
  lastBtn = b;
}

void handleInputs() {
  quietMode = (digitalRead(SWITCH_PIN) == LOW);
  handleTouchPetting();
  handleButtonNav();
}

#include "DeskPalNetwork.h"


bool currentTm(struct tm* t) {
  if (timeSynced) {
    time_t epoch; time(&epoch);
    return localtime_r(&epoch, t) != nullptr;
  }
  time_t demo = demoBaseEpoch + millis() / 1000;
  return gmtime_r(&demo, t) != nullptr;
}

#include "DeskPalDisplay.h"

void setup() {
  Serial.begin(115200);
  esp_err_t wdtResult = esp_task_wdt_init(30, true);
  if (wdtResult == ESP_OK || wdtResult == ESP_ERR_INVALID_STATE) {
    esp_err_t addResult = esp_task_wdt_add(NULL);
    watchdogActive = addResult == ESP_OK || addResult == ESP_ERR_INVALID_STATE;
  }
  if (!watchdogActive) Serial.println("Warning: task watchdog unavailable");
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("I2C scan:");
  for (byte a = 1; a < 127; a++) { Wire.beginTransmission(a); if (Wire.endTransmission() == 0) { Serial.print("  found 0x"); Serial.println(a, HEX); } }
  pinMode(TOUCH_PIN, INPUT); pinMode(BTN_MICMUTE_PIN, INPUT_PULLUP); pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
#if ENABLE_BATTERY
  pinMode(BATT_PIN, INPUT); analogReadResolution(12);
#endif
#if USE_SH1106
  display.begin(0x3C, true);
#else
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
#endif
  display.setTextColor(OLED_WHITE);
  bool forceConfig = false;
  for (unsigned long t = millis(); millis() - t < CONFIG_HOLD_MS;) {
    
    if (digitalRead(TOUCH_PIN) == HIGH) { forceConfig = true; break; }
    delay(80);
  }
  loadConfig();
#if ENABLE_WEATHER
  loadTelemetryToken();
#endif
#if ENABLE_WEATHER
  if (forceConfig) { startConfigPortal(); return; }
#endif
  leftEye.init(18, 14, 36, 36); rightEye.init(74, 14, 36, 36);
  playBootAnimation();
  bmpOk = bme.begin() || bme.begin(0x76);
  Serial.println(bmpOk ? "BME280 OK" : "BME280 NOT FOUND - check wiring!");
  if (bmpOk) {
    for (int i = 0; i < 5; i++) { bme.readTemperature(); bme.readHumidity(); bme.readPressure(); delay(40); }
    Serial.println("=== TEMP CALIBRATION ===");
    Serial.print("Raw: "); Serial.println(bme.readTemperature());
    Serial.print("Adj: "); Serial.println(bme.readTemperature() + TEMP_OFFSET);
  }
  lastInteraction = millis(); readSensors(); generateInsights(); batteryPct = readBatteryPct();
  lastSensorRead = millis(); lastInsightGen = millis(); lastPageSwitch = millis();
  initMic();
#if ENABLE_WEATHER
  if (wifiSsid.length() > 0) {
    wifiWant = true; WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.setAutoReconnect(true); WiFi.persistent(false);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str()); wifiRetryAt = millis() + 30000;
    Serial.println("WiFi: connecting in background...");
  } else { Serial.println("WiFi: no credentials saved (hold touch @ boot to configure)"); }
#endif
  buzzBoot(); Serial.println("{\"type\":\"decision\",\"msg\":\"Boot complete\"}");
}

void updateTimer() {
  if (timerRunning) {
    unsigned long now = millis();
    if (now - timerLastTick >= 1000) {
      timerRemainingSec--;
      timerLastTick = now;
      if (timerRemainingSec == 0) {
        timerRunning = false;
        timerLock = false;
        isTimerAlarm = true;
        excitedUntil = now + 5000;
        addInsightOnce("Study session complete! Take a stretch break!");
      }
    }
  }
  if (isTimerAlarm) {
    static unsigned long lastBeep = 0;
    if (millis() - lastBeep > 800) { buzzAlert(); lastBeep = millis(); }
  }
}

void serviceWatchdog() {
  if (watchdogActive) esp_task_wdt_reset();
}

void loop() {
  serviceWatchdog();
  if (inConfigMode) {
#if ENABLE_WEATHER
    configServer.handleClient();
#endif
    return;
  }
  unsigned long now = millis();
  if (!screenOff && now - lastInteraction > OLED_TIMEOUT_MS) { DISPLAY_OFF(); screenOff = true; }
  handleInputs(); liftCheck(); processAudio(); updateTimer(); updateBuzzer();
#if ENABLE_WEATHER
  wifiTick(now);
  if (telemetryServerStarted) configServer.handleClient();
#endif
  if (now - lastSensorRead > 2000) { readSensors(); lastSensorRead = now; }
  if (now - lastInsightGen > 10000) { generateInsights(); lastInsightGen = now; }
#if ENABLE_WEATHER
  if (wifiOn && timeSynced && city.length() > 0 && apiKey.length() > 0) {
    WeatherSnapshot weather = getWeatherSnapshot();
    unsigned long interval = weather.ok ? 600000UL : 60000UL;
    if (now - lastWeatherUpdate > interval && requestWeatherFetch()) lastWeatherUpdate = now;
  }
#endif
  computeMood(); emitDashboardTelemetry();
  if (screenOff) return;
  display.clearDisplay();
  switch (currentPage) {
    case PAGE_EYES: drawEmoPage(); break;
    case PAGE_CLOCK: drawClock(); break;
    case PAGE_ENV: drawEnvPage(); break;
    case PAGE_INSIGHTS: drawInsightsPage(); break;
    case PAGE_KWS: drawKwsPage(); break;
#if ENABLE_WEATHER
    case PAGE_WEATHER: drawWeatherCard(); break;
    case PAGE_FORECAST: drawForecastPage(); break;
#endif
    case PAGE_WORLD: drawWorldClock(); break;
    case PAGE_TIMER: drawTimerPage(); break;
  }
  drawTimerOverlay();
  display.display();
}
