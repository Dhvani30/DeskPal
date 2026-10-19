// ==================================================
// DeskPal - ESP32 desktop companion firmware
// ==================================================
// The keyword model is not bundled. The mic reports levels only by default;
// loud sounds are deliberately not treated as a wake phrase.
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


// --- Inlined from DeskPalConfig.h ---

// Board and feature configuration. Select the profile matching the hardware.
#ifndef USE_SH1106
#define USE_SH1106   1
#endif
#if USE_SH1106
#include <Adafruit_SH110X.h>
#else
#include <Adafruit_SSD1306.h>
#endif

#ifndef ENABLE_WEATHER
#define ENABLE_WEATHER 1
#endif
#if ENABLE_WEATHER
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- Inlined from DeskPalTrust.h ---

#include <Arduino.h>

// ISRG Root X1 trust anchor. Keep this CA current during device maintenance.
static const char OPENWEATHER_ROOT_CA[] PROGMEM = R"cert(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----)cert";

// --- End DeskPalTrust.h ---

#endif

#include <Adafruit_BME280.h>

// --- Inlined from DeskPalAssets.h ---

// Small monochrome weather and mood icons stored in flash.
const unsigned char bmp_clear[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0xc0,0x80,0x00,0x0f,0xf0,0x00,0x00,0x3f,0xfc,0x00,0x00,0x7f,0xfe,0x00,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x00,0x7f,0xfe,0x00,0x00,0x3f,0xfc,0x00,0x01,0x0f,0xf0,0x80,0x00,0x03,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_clouds[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x3f,0xff,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x00,0xff,0xff,0xe0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_rain[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x0c,0x00,0x00,0x60,0x0c,0x00,0x00,0xe0,0x1c,0x00,0x00,0xc0,0x18,0x00,0x03,0x80,0x70,0x00,0x03,0x80,0x70,0x00,0x03,0x00,0x60,0x00,0x02,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char mini_sun[] PROGMEM = {0x00,0x00,0x01,0x80,0x00,0x00,0x10,0x08,0x04,0x20,0x03,0xc0,0x27,0xe4,0x07,0xe0,0x07,0xe0,0x27,0xe4,0x03,0xc0,0x04,0x20,0x10,0x08,0x00,0x00,0x01,0x80,0x00,0x00};
const unsigned char mini_cloud[] PROGMEM = {0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x0f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char mini_rain[] PROGMEM = {0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x00,0x00,0x44,0x44,0x22,0x22,0x11,0x11};
const unsigned char bmp_tiny_drop[] PROGMEM = {0x10,0x38,0x7c,0xfe,0xfe,0x7c,0x38,0x00};
const unsigned char bmp_heart[] PROGMEM = {0x00,0x00,0x0c,0x60,0x1e,0xf0,0x3f,0xf8,0x7f,0xfc,0x7f,0xfc,0x7f,0xfc,0x3f,0xf8,0x1f,0xf0,0x0f,0xe0,0x07,0xc0,0x03,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_zzz[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,0x30,0x00,0x7e,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,0x30,0x00,0x7c,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_anger[] PROGMEM = {0x00,0x00,0x11,0x10,0x2a,0x90,0x44,0x40,0x80,0x20,0x80,0x20,0x44,0x40,0x2a,0x90,0x11,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_music[] PROGMEM = {0x01,0x80,0x01,0x80,0x01,0x80,0x01,0xc0,0x01,0xe0,0x03,0xf0,0x07,0xe0,0x0f,0xc0,0x07,0x80,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_snow[] PROGMEM = {0x99,0x5A,0x3C,0xFF,0xFF,0x3C,0x5A,0x99};

// --- End DeskPalAssets.h ---


#ifndef USE_TFLITE
#define USE_TFLITE 0
#endif
#ifndef KWS_MODEL_INTEGRATED
#define KWS_MODEL_INTEGRATED 0
#endif
#define KWS_CONFIDENCE_FLOOR 0.75f
#define KWS_LABEL_REJECT -1
#define KWS_LABEL_WAKE 0
#define KWS_LABEL_START_TIMER 1
#define KWS_LABEL_STOP_TIMER 2
#if USE_TFLITE

// --- Inlined from DeskPalKwsAdapter.h ---

// Integration seam for the generated Edge Impulse Arduino library.
// Replace this contract after exporting the trained model. Do not set the
// readiness macro until the implementation has passed the KWS acceptance tests.
#define DESKPAL_KWS_ADAPTER_READY 0

// Return KWS_LABEL_WAKE / KWS_LABEL_START_TIMER / KWS_LABEL_STOP_TIMER only
// when the model confidence meets KWS_CONFIDENCE_FLOOR; otherwise return
// KWS_LABEL_REJECT. The implementation must consume a complete model window
// from the 16 kHz mono I2S stream and must not retain a pointer to DMA memory.
int classifyKeywordEdge();

// --- End DeskPalKwsAdapter.h ---

#if !KWS_MODEL_INTEGRATED || !DESKPAL_KWS_ADAPTER_READY
#error "USE_TFLITE requires an exported Edge Impulse model, adapter implementation, and KWS_MODEL_INTEGRATED=1."
#endif
#endif

#ifndef USE_ESP32_S3
#define USE_ESP32_S3 0
#endif
#if USE_ESP32_S3
#define SDA_PIN 8
#define SCL_PIN 9
#define TOUCH_PIN 7
#define BUZZER_PIN 6
#define SWITCH_PIN 5
#define BTN_MICMUTE_PIN 4
#define MIC_SCK 15
#define MIC_WS 16
#define MIC_SD 17
#define BATT_PIN 1
#else
#define SDA_PIN 21
#define SCL_PIN 22
#define TOUCH_PIN 32
#define BUZZER_PIN 27
#define SWITCH_PIN 26
#define BTN_MICMUTE_PIN 25
#define MIC_SCK 18
#define MIC_WS 19
#define MIC_SD 35
#define BATT_PIN 34
#endif

#ifndef ENABLE_MIC
#define ENABLE_MIC 1
#endif
#ifndef ENABLE_BATTERY
#define ENABLE_BATTERY 0
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TEMP_OFFSET -1.5f
#define NEGLECT_MS 120000UL
#define LIFT_THRESHOLD 0.10f
#define PET_LIMIT 8
#define PET_WINDOW_MS 15000UL
#define RUB_WINDOW_MS 1200UL
#define RUB_MIN_TRANSITIONS 4
#define BATT_LOW_PCT 15

#define BTN_LONG_PRESS_MS        1500UL
#define TOUCH_LONG_HOLD_MS       1500UL
#define OLED_TIMEOUT_MS          0UL // 0 keeps the OLED on; the small LiPo will run down faster.
#define TIMER_MIN_MINS           10


#define CLIMATE_TEMPERATE 0
#define CLIMATE_COASTAL 1
#define CLIMATE_DESERT 2
#define CLIMATE_MOUNTAIN 3

#define MOOD_COUNT 11
#define MOOD_NORMAL 0
#define MOOD_HAPPY 1
#define MOOD_EXCITED 2
#define MOOD_LOVE 3
#define MOOD_ANGRY 4
#define MOOD_SCARED 5
#define MOOD_SURPRISED 6
#define MOOD_SLEEPY 7
#define MOOD_COLD 8
#define MOOD_SAD 9
#define MOOD_SUSPICIOUS 10

#define PAGE_EYES 0
#define PAGE_CLOCK 1
#define PAGE_ENV 2
#define PAGE_INSIGHTS 3
#define PAGE_KWS 4
#define PAGE_WEATHER 5
#define PAGE_FORECAST 6
#define PAGE_WORLD 7
#define PAGE_TIMER 8

#define SENSOR_GRACE_MS 8000UL
#define SENSOR_RECOVERY_FAILS 5
#define COMFORT_OK 0
#define COMFORT_HOT 1
#define COMFORT_COLD 2
#define COMFORT_HUMID 3
#define COMFORT_DRY 4

#define CONFIG_HOLD_MS 3000
#define CONFIG_AP_PIN_LENGTH 8


// --- End DeskPalConfig.h ---


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
bool micMuted = true; // Explicit user action is required before reading microphone samples.
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
unsigned long lastAnomalyNoticeAt = 0;
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

// Classify only when a measured value crosses the selected comfort band.
int classifyComfort(float T, float H) {
  ClimateParams z = getClimateParams();
  const float tMin = z.tMid - z.tHalf, tMax = z.tMid + z.tHalf;
  const float hMin = z.hMid - z.hHalf, hMax = z.hMid + z.hHalf;
  if (T > tMax) return COMFORT_HOT;
  if (T < tMin) return COMFORT_COLD;
  if (H > hMax) return COMFORT_HUMID;
  if (H < hMin) return COMFORT_DRY;
  return COMFORT_OK;
}
void updateOnlineAnomalyModel(float t, float h, float p) {
  float f[3] = { t, h, p };
  if (kmSamples == 0) { centroid[0] = t; centroid[1] = h; centroid[2] = p; }
  if (kmSamples < 60) {
    for (int i = 0; i < 3; i++) {
      float d = fabsf(f[i] - centroid[i]);
      centroid[i] += 0.05f * (f[i] - centroid[i]);
      spread[i] += 0.08f * (d - spread[i]);
    }
    kmSamples++;
    return;
  }
  float dT = fabsf(f[0] - centroid[0]);
  float dH = fabsf(f[1] - centroid[1]);
  float dP = fabsf(f[2] - centroid[2]);
  anomalyMsg[0] = '\0';
  const float tLimit = fmaxf(2.5f, spread[0] * 5.0f);
  const float hLimit = fmaxf(10.0f, spread[1] * 5.0f);
  const float pLimit = fmaxf(5.0f, spread[2] * 5.0f);
  if (dT > tLimit) strlcpy(anomalyMsg, "Room temperature changed quickly", sizeof(anomalyMsg));
  else if (dH > hLimit) strlcpy(anomalyMsg, "Room humidity changed quickly", sizeof(anomalyMsg));
  else if (dP > pLimit && millis() > liftSuppressUntil) strlcpy(anomalyMsg, "Room pressure changed quickly", sizeof(anomalyMsg));
  if (anomalyMsg[0] != '\0' && (lastAnomalyNoticeAt == 0 || millis() - lastAnomalyNoticeAt >= 60000UL)) {
    anomalyUntil = millis() + 5000UL;
    lastAnomalyNoticeAt = millis();
    Serial.print("{\"type\":\"decision\",\"msg\":\""); Serial.print(anomalyMsg); Serial.println("\"}");
  } else {
    anomalyMsg[0] = '\0';
  }
  if (anomalyMsg[0] == '\0') {
    for (int i = 0; i < 3; i++) centroid[i] += 0.01f * (f[i] - centroid[i]);
    spread[0] += 0.02f * (dT - spread[0]);
    spread[1] += 0.02f * (dH - spread[1]);
    spread[2] += 0.02f * (dP - spread[2]);
  }
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
    comfortClass = classifyComfort(envTemp, envHum);
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
    if (wifiOn && weather.ok && sensorsOk && !isnan(envTemp)) {
      ClimateParams localComfort = getClimateParams();
      const float roomMin = localComfort.tMid - localComfort.tHalf;
      const float roomMax = localComfort.tMid + localComfort.tHalf;
      // Compare the room to the configured city's current outdoor temperature.
      // The pet reacts only when both the outdoor condition and room reading
      // point to a real hot/cold situation, not just a normal indoor/outdoor gap.
      if (weather.temperature > roomMax && envTemp > roomMax) m = MOOD_ANGRY;
      else if (weather.temperature < roomMin && envTemp < roomMin) m = MOOD_COLD;
      else if (weather.temperature > roomMax && envTemp <= roomMax) m = MOOD_HAPPY;
      else if (weather.temperature < roomMin && envTemp >= roomMin) m = MOOD_HAPPY;
      else if (strcmp(weather.main, "Rain") == 0 || strcmp(weather.main, "Drizzle") == 0) m = MOOD_SAD;
      else if (strcmp(weather.main, "Thunderstorm") == 0) m = MOOD_SURPRISED;
      else if (strcmp(weather.main, "Clear") == 0 && comfortClass == COMFORT_OK) m = MOOD_HAPPY;
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


// --- Inlined from DeskPalAudio.h ---

// Microphone capture, wake-word contract, and voice command behavior.
void triggerMiniAI() {
  isAiResponding = true; aiResponseStart = millis(); currentPage = PAGE_KWS;
  if (!sensorsOk && !timerRunning && !anomalyActive) {
    strlcpy(aiResponse, "Room sensor unavailable; check connections.", sizeof(aiResponse));
    return;
  }
  float sHot = sensorsOk && !isnan(envTemp) ? cf((envTemp - 27.0f) / 3.0f, 0, 1) : 0.0f;
  float sCold = sensorsOk && !isnan(envTemp) ? cf((22.0f - envTemp) / 3.0f, 0, 1) : 0.0f;
  float sHum = sensorsOk && !isnan(envHum) ? cf((envHum - 75.0f) / 15.0f, 0, 1) : 0.0f;
  float sDry = sensorsOk && !isnan(envHum) ? cf((30.0f - envHum) / 15.0f, 0, 1) : 0.0f;
  float sNoise = cf((micRms - micNoiseFloor * 2.0f) / (micNoiseFloor * 3.0f), 0, 1);
  float sQuiet = (micReady ? 1.0f : 0.0f) * cf((200.0f - micRms) / 200.0f, 0, 1);
  float sRain = (!isnan(envPress) && !isnan(pressHourAgo)) ? cf((pressHourAgo - envPress) / 2.0f, 0, 1) : 0.0f;
  float sTimer = timerRunning ? 1.0f : 0.0f;
  float sAnom = anomalyActive ? 1.0f : 0.0f;
  char cands[8][72]; float scores[8];
  snprintf(cands[0], sizeof(cands[0]), "Timer running: %lu min left. Focus!", timerRemainingSec / 60); scores[0] = sTimer;
  snprintf(cands[1], sizeof(cands[1]), "Env shifting! %s", anomalyMsg); scores[1] = sAnom;
  if (sensorsOk && !isnan(envTemp)) {
    snprintf(cands[2], sizeof(cands[2]), "It's %dC - warm. Hydrate, human!", (int)envTemp);
    snprintf(cands[3], sizeof(cands[3]), "It's %dC - chilly. Grab a jacket!", (int)envTemp);
  } else {
    strlcpy(cands[2], "Room sensor unavailable.", sizeof(cands[2]));
    strlcpy(cands[3], "Room sensor unavailable.", sizeof(cands[3]));
  }
  if (sensorsOk && !isnan(envHum)) snprintf(cands[4], sizeof(cands[4]), "Humidity %d%% - fan time!", (int)envHum);
  else strlcpy(cands[4], "Room sensor unavailable.", sizeof(cands[4]));
  scores[2] = sHot; scores[3] = sCold; scores[4] = sHum;
  snprintf(cands[5], sizeof(cands[5]), "Air is dry - sip some water!"); scores[5] = sDry;
  snprintf(cands[6], sizeof(cands[6]), "Room noisy - find a quiet spot!"); scores[6] = sNoise;
  if (sRain > 0.3f) snprintf(cands[7], sizeof(cands[7]), "Pressure falling - rain likely soon.");
  else snprintf(cands[7], sizeof(cands[7]), "Room quiet & stable. Perfect for deep work!");
  scores[7] = max(sQuiet, sRain * 0.9f);
  int best = 0;
  for (int i = 1; i < 8; i++) if (scores[i] > scores[best]) best = i;
  if (scores[best] < 0.15f) best = 7;
  strlcpy(aiResponse, cands[best], sizeof(aiResponse));
  Serial.print("{\"type\":\"decision\",\"msg\":\"MiniAI: "); Serial.print(aiResponse); Serial.println("\"}");
}

void processVoiceCommand() {
  isAiResponding = true; aiResponseStart = millis(); currentPage = PAGE_KWS;
  // Counting loud syllables is not speech recognition. Never start or stop a
  // timer from that guess; leave timer control to the physical button.
  if (commandSyllables == 0) strlcpy(aiResponse, "No speech heard. Timer unchanged.", sizeof(aiResponse));
  else strlcpy(aiResponse, "Speech not recognized. Timer unchanged.", sizeof(aiResponse));
}

void keywordSpotted() {
  kwsActiveUntil = millis() + 2500; excitedUntil = millis() + 2500; lastInteraction = millis(); buzzKws();
  Serial.println("{\"type\":\"decision\",\"msg\":\"Wake word detected\"}");
#if USE_TFLITE
  isListeningForCommand = false;
  strlcpy(aiResponse, "Wake word recognized", sizeof(aiResponse));
#else
  isListeningForCommand = true; listenStartTime = millis(); lastCmdSyllableAt = 0; commandSyllables = 0; cmdInSyllable = false;
  strlcpy(aiResponse, "Listening... (speak!)", sizeof(aiResponse));
#endif
  isAiResponding = true; aiResponseStart = millis(); currentPage = PAGE_KWS;
}

#if !USE_TFLITE
int classifyKeywordEdge() { return KWS_LABEL_REJECT; }
#endif
#if USE_TFLITE
void handleKeywordLabel(int label) {
  unsigned long now = millis();
  if (label == KWS_LABEL_REJECT || now - lastKwsActionAt < 1200) return;
  lastKwsActionAt = now;
  if (label == KWS_LABEL_WAKE) { keywordSpotted(); return; }
  if (label == KWS_LABEL_START_TIMER) {
    if (timerRemainingSec == 0) timerRemainingSec = timerDurationSec;
    timerRunning = true; timerLock = true; timerLastTick = now;
    strlcpy(aiResponse, "Focus timer started", sizeof(aiResponse)); buzzHappy();
    Serial.println("{\"type\":\"decision\",\"msg\":\"Voice command: start timer\"}");
  } else if (label == KWS_LABEL_STOP_TIMER) {
    timerRunning = false; timerLock = false;
    strlcpy(aiResponse, "Focus timer stopped", sizeof(aiResponse)); buzzTap();
    Serial.println("{\"type\":\"decision\",\"msg\":\"Voice command: stop timer\"}");
  } else return;
  isAiResponding = true; aiResponseStart = now; currentPage = PAGE_KWS;
}
#endif

void petAction(unsigned long now, bool strongRub) {
  lastInteraction = now;
  if (neglectSleepy) { excitedUntil = now + 3000; buzzHappy(); return; }
  if (now < annoyedUntil) { annoyedUntil = now + 4000; buzzGrumble(); return; }
  if (now - petWindowStart > PET_WINDOW_MS) { petWindowStart = now; petCount = 0; }
  petCount++;
  if (petCount >= PET_LIMIT) { annoyedUntil = now + 5000; petCount = 0; buzzGrumble(); Serial.println("{\"type\":\"decision\",\"msg\":\"Over-petted - angry\"}"); }
  else { pettedUntil = now + (strongRub ? 5000UL : 3000UL); buzzHappy(); }
}
void longPetAction(unsigned long now) {
  lastInteraction = now; annoyedUntil = 0; petCount = 0; pettedUntil = now + 6000; buzzHappy();
  Serial.println("{\"type\":\"decision\",\"msg\":\"Long cuddle - calmed\"}");
}

void initMic() {
#if ENABLE_MIC
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = 16000;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 4; cfg.dma_buf_len = 256;
  i2s_pin_config_t pins = {};
  pins.bck_io_num = MIC_SCK; pins.ws_io_num = MIC_WS; pins.data_in_num = MIC_SD; pins.data_out_num = I2S_PIN_NO_CHANGE;
  if (i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL) == ESP_OK && i2s_set_pin(I2S_NUM_0, &pins) == ESP_OK) {
    micReady = true; Serial.println("INMP441 I2S mic ready");
  } else { Serial.println("INMP441 init failed"); }
#else
  Serial.println("Mic disabled (ENABLE_MIC 0)");
#endif
}
float readMicRms() {
  if (!micReady || micMuted) return 0.0f;
  size_t bytesRead = 0;
  i2s_read(I2S_NUM_0, i2sBuf, sizeof(i2sBuf), &bytesRead, 20);
  if (bytesRead == 0) return micRms;
  int n = bytesRead / 4; double sum = 0;
  for (int i = 0; i < n; i++) { int16_t s = (int16_t)(i2sBuf[i] >> 16); sum += (double)s * s; }
  float rms = sqrtf((float)(sum / n));
  micRms = micRms * 0.7f + rms * 0.3f;
  micNoiseFloor = micNoiseFloor * 0.99f + micRms * 0.01f;
  return micRms;
}
bool detectHeyPalEnergy(float rms) {
  if (micMuted) return false;
  float thresh = micNoiseFloor * 4.5f + 500.0f;
  unsigned long now = millis();
  if (isListeningForCommand) {
    if (now - listenStartTime > 3000) { isListeningForCommand = false; processVoiceCommand(); return false; }
    if (!cmdInSyllable && rms > thresh && now - lastCmdSyllableAt > 180) { cmdInSyllable = true; commandSyllables++; lastCmdSyllableAt = now; }
    else if (cmdInSyllable && rms < thresh * 0.45f) { cmdInSyllable = false; }
    return false;
  }
  switch (kwsState) {
    case KWS_IDLE: if (rms > thresh) { kwsState = KWS_SYL1; kwsSyl1Time = now; } break;
    case KWS_SYL1: if (now - kwsSyl1Time > 120 && rms < thresh * 0.45f) kwsState = KWS_GAP; if (now - kwsSyl1Time > 450) kwsState = KWS_IDLE; break;
    case KWS_GAP: if (rms > thresh && now - kwsSyl1Time > 180 && now - kwsSyl1Time < 900) { kwsState = KWS_IDLE; return true; } if (now - kwsSyl1Time > 950) kwsState = KWS_IDLE; break;
  }
  return false;
}
void processAudio() {
  if (!micReady || micMuted) return;
  readMicRms(); // Audio level only unless a real keyword model is integrated.
#if USE_TFLITE
  int label = classifyKeywordEdge();
  handleKeywordLabel(label);
#endif
}

// --- End DeskPalAudio.h ---


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
  const unsigned long now = millis();
  static bool rawTouch = false;
  static unsigned long rawTouchChangedAt = 0;
  const bool sampledTouch = (digitalRead(TOUCH_PIN) == HIGH);
  if (sampledTouch != rawTouch) { rawTouch = sampledTouch; rawTouchChangedAt = now; return; }
  if (now - rawTouchChangedAt < 40UL) return; // ignore short electrical glitches
  const bool t = rawTouch;

  if (t && !lastTouch) {
    if (wakeScreenIfOff(now)) { lastTouch = t; return; }
    recordTouchTransition(now);
    touchPressStart = now;
    touchLongHoldFired = false;
  } else if (t && lastTouch) {
    if (!touchLongHoldFired && now - touchPressStart >= TOUCH_LONG_HOLD_MS) {
      touchLongHoldFired = true;
      lastInteraction = now;
      if (currentPage == PAGE_TIMER) {
        timerRunning = false;
        timerLock = false;
        timerDurationSec = TIMER_MIN_MINS * 60UL;
        timerRemainingSec = timerDurationSec;
        buzzTap();
      } else {
        longPetAction(now);
      }
    }
  } else if (!t && lastTouch) {
    if (!touchLongHoldFired) {
      const unsigned long heldMs = now - touchPressStart;
      const bool strongRub = (touchTransitions >= RUB_MIN_TRANSITIONS);
      if (heldMs < TOUCH_LONG_HOLD_MS) {
        if (currentPage == PAGE_TIMER) {
          // On the timer page, the touch pad adjusts time instead of petting.
          if (!timerRunning && !isTimerAlarm) {
            timerDurationSec = min(timerDurationSec + 5UL * 60UL, 120UL * 60UL);
            timerRemainingSec = timerDurationSec;
            lastInteraction = now;
            buzzTap();
          }
        } else {
          petAction(now, strongRub);
        }
      }
    }
    touchTransitions = 0;
    rubWindowStart = 0;
  }
  lastTouch = t;
}

void clearTimerAlarm() {
  isTimerAlarm = false;
  timerRunning = false;
  timerLock = false;
  timerDurationSec = TIMER_MIN_MINS * 60UL;
  timerRemainingSec = timerDurationSec;
  digitalWrite(BUZZER_PIN, LOW);
  buzzQueueIdx = -1;
  buzzQueueLen = 0;
}

void handleButtonNav() {
  const unsigned long now = millis();
  static bool rawPressed = false;
  static unsigned long rawButtonChangedAt = 0;
  const bool sampledPressed = (digitalRead(BTN_MICMUTE_PIN) == LOW);
  if (sampledPressed != rawPressed) { rawPressed = sampledPressed; rawButtonChangedAt = now; return; }
  if (now - rawButtonChangedAt < 30UL) return; // debounce the physical button
  const bool b = rawPressed;

  if (b && !lastBtn) {
    lastInteraction = now;
    if (wakeScreenIfOff(now)) { lastBtn = b; return; }
    btnPressStart = now;
    btnLongHandled = false;
    if (isTimerAlarm) {
      clearTimerAlarm();
      btnLongHandled = true;
      buzzTap();
    }
  } else if (b && lastBtn) {
    const unsigned long heldMs = now - btnPressStart;
    if (!btnLongHandled && heldMs >= BTN_LONG_PRESS_MS) {
      btnLongHandled = true;
      lastInteraction = now;
      if (currentPage == PAGE_TIMER) {
        clearTimerAlarm();
        currentPage = PAGE_EYES;
        buzzTap();
      } else if (currentPage == PAGE_EYES) {
        micMuted = !micMuted;
        Serial.println(String("{\"type\":\"decision\",\"msg\":\"Mic ") + (micMuted ? "muted" : "unmuted (level monitor only; speech model not installed)") + "\"}");
        buzzTap();
      } else {
        currentPage = PAGE_EYES;
        lastSaccade = 0;
        buzzTap();
      }
    }
  } else if (!b && lastBtn) {
    const unsigned long heldMs = now - btnPressStart;
    if (!btnLongHandled && heldMs < BTN_LONG_PRESS_MS) {
      lastInteraction = now;
      if (currentPage == PAGE_TIMER) {
        if (timerRunning) {
          timerRunning = false; // pause and keep the remaining time
          timerLock = false;
          buzzTap();
        } else if (!isTimerAlarm) {
          if (timerRemainingSec == 0) timerRemainingSec = timerDurationSec;
          timerRunning = true;
          timerLock = true;
          timerLastTick = now;
          buzzTap();
        }
      } else {
        currentPage++;
        if (currentPage > maxMainPage()) currentPage = 0;
        lastSaccade = 0;
        buzzTap();
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


// --- Inlined from DeskPalNetwork.h ---

// Local authenticated telemetry API and asynchronous weather networking.
#if ENABLE_WEATHER
void getWeatherAndForecast();
WeatherSnapshot getWeatherSnapshot() {
  WeatherSnapshot copy;
  portENTER_CRITICAL(&weatherStateMux);
  copy = weatherState;
  portEXIT_CRITICAL(&weatherStateMux);
  return copy;
}

void publishWeatherSnapshot(const WeatherSnapshot& snapshot) {
  portENTER_CRITICAL(&weatherStateMux);
  weatherState = snapshot;
  portEXIT_CRITICAL(&weatherStateMux);
}

void weatherFetchTask(void* parameter) {
  (void)parameter;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    getWeatherAndForecast();
    portENTER_CRITICAL(&weatherStateMux);
    weatherFetchInFlight = false;
    portEXIT_CRITICAL(&weatherStateMux);
  }
}

bool requestWeatherFetch() {
  portENTER_CRITICAL(&weatherStateMux);
  if (weatherFetchInFlight) { portEXIT_CRITICAL(&weatherStateMux); return false; }
  weatherFetchInFlight = true;
  portEXIT_CRITICAL(&weatherStateMux);
  if (!weatherTaskHandle) {
    BaseType_t result = xTaskCreatePinnedToCore(weatherFetchTask, "deskpal-weather", 8192, nullptr, 1, &weatherTaskHandle, 0);
    if (result != pdPASS) {
      weatherTaskHandle = nullptr;
      portENTER_CRITICAL(&weatherStateMux); weatherFetchInFlight = false; portEXIT_CRITICAL(&weatherStateMux);
      Serial.println("Weather worker start failed");
      return false;
    }
  }
  xTaskNotifyGive(weatherTaskHandle);
  return true;
}

void loadTelemetryToken() {
  if (!prefs.begin("deskpal", false)) return;
  String token = prefs.getString("apiToken", "");
  if (token.length() != 32) {
    char generated[33];
    for (int i = 0; i < 4; ++i) snprintf(generated + i * 8, 9, "%08lx", (unsigned long)esp_random());
    token = generated;
    prefs.putString("apiToken", token);
  }
  strlcpy(telemetryToken, token.c_str(), sizeof(telemetryToken));
  prefs.end();
}

bool tokenMatches(const String& candidate) {
  size_t candidateLength = candidate.length();
  size_t expectedLength = strlen(telemetryToken);
  size_t difference = candidateLength ^ expectedLength;
  for (size_t i = 0; i < expectedLength; ++i) {
    char received = i < candidateLength ? candidate[i] : 0;
    difference |= (size_t)(uint8_t)(received ^ telemetryToken[i]);
  }
  return difference == 0 && expectedLength == 32;
}

void addApiCorsHeaders() {
  configServer.sendHeader("Access-Control-Allow-Origin", "*");
  configServer.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  configServer.sendHeader("Access-Control-Allow-Headers", "X-DeskPal-Token");
  configServer.sendHeader("Cache-Control", "no-store");
}

void sendTelemetryApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders();
    configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}");
    return;
  }
  String body;
  body.reserve(1100);
  body = "{\"type\":\"telemetry\",\"schema\":1,\"uptimeMs\":"; body += millis();
  body += ",\"sensorsOk\":"; body += sensorsOk ? "true" : "false";
  body += ",\"sensorAgeMs\":";
  if (lastGoodSensorRead) body += String(millis() - lastGoodSensorRead); else body += "null";
  body += ",\"micReady\":"; body += micReady ? "true" : "false";
  body += ",\"modelReady\":"; body += KWS_MODEL_INTEGRATED ? "true" : "false";
  body += ",\"t\":"; body += (!sensorsOk || isnan(envTemp)) ? "null" : String(envTemp, 1);
  body += ",\"h\":"; body += (!sensorsOk || isnan(envHum)) ? "null" : String((int)envHum);
  body += ",\"p\":"; body += (!sensorsOk || isnan(envPress)) ? "null" : String((int)envPress);
  body += ",\"mood\":\""; body += MOOD_NAMES[currentMood];
  body += "\",\"comfort\":\""; body += comfortNames[comfortClass];
  body += "\",\"infMs\":"; body += String(infAvg, 2);
  body += ",\"batt\":"; body += isnan(batteryPct) ? "null" : String(batteryPct, 0);
  body += ",\"micMute\":"; body += micMuted ? "true" : "false";
  body += ",\"micRms\":"; body += String(micRms, 0);
  body += ",\"anomaly\":"; body += anomalyActive ? "true" : "false";
  body += ",\"climate\":"; body += climateZone; body += ",\"histT\":[";
  int n = histFilled ? 60 : histIdx;
  for (int i = 0; i < n; ++i) {
    int idx = histFilled ? (histIdx + i) % 60 : i;
    if (i) body += ',';
    body += String(histTemp[idx], 1);
  }
  body += "]}";
  addApiCorsHeaders();
  configServer.send(200, "application/json", body);
}

void handleCorsOptions() {
  addApiCorsHeaders();
  configServer.send(204);
}
#endif

#if ENABLE_WEATHER
const unsigned char* getBigIcon(const char* w) {
  if (strcmp(w, "Clear") == 0) return bmp_clear;
  if (strcmp(w, "Rain") == 0 || strcmp(w, "Drizzle") == 0) return bmp_rain;
  return bmp_clouds;
}
const unsigned char* getMiniIcon(const char* w) {
  if (strcmp(w, "Clear") == 0) return mini_sun;
  if (strcmp(w, "Rain") == 0 || strcmp(w, "Drizzle") == 0 || strcmp(w, "Thunderstorm") == 0) return mini_rain;
  return mini_cloud;
}
static bool owmUseCountry = true;

static void trimUrlPart(const String& src, char* dst, size_t n) {
  int start = 0, end = (int)src.length();
  while (start < end && src[start] == ' ') start++;
  while (end > start && src[end - 1] == ' ') end--;
  size_t out = 0;
  static const char hex[] = "0123456789ABCDEF";
  for (int i = start; i < end; ++i) {
    unsigned char ch = (unsigned char)src[i];
    if (isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
      if (out + 1 >= n) break;
      dst[out++] = (char)ch;
    } else if (ch == ' ') {
      if (out + 1 >= n) break;
      dst[out++] = '+';
    } else {
      if (out + 3 >= n) break;
      dst[out++] = '%'; dst[out++] = hex[ch >> 4]; dst[out++] = hex[ch & 0x0F];
    }
  }
  dst[out] = '\0';
}

static void buildOwmUrl(char* out, size_t n, const char* path, const char* cityQ, bool withCountry) {
  char cc[16], key[384];
  trimUrlPart(countryCode, cc, sizeof(cc));
  trimUrlPart(apiKey, key, sizeof(key));
  if (withCountry && strlen(cc) == 2) {
    snprintf(out, n, "https://api.openweathermap.org/data/2.5/%s?q=%s,%s&appid=%s&units=metric",
             path, cityQ, cc, key);
  } else {
    snprintf(out, n, "https://api.openweathermap.org/data/2.5/%s?q=%s&appid=%s&units=metric",
             path, cityQ, key);
  }
}

void getWeatherAndForecast() {
  if (WiFi.status() != WL_CONNECTED || !timeSynced) return;
  WeatherSnapshot next = getWeatherSnapshot();
  char cityQ[193];
  trimUrlPart(city, cityQ, sizeof(cityQ));
  if (cityQ[0] == '\0' || apiKey.isEmpty()) return;
  char cc[16]; trimUrlPart(countryCode, cc, sizeof(cc));
  if (strlen(cc) != 2) owmUseCountry = false;

  char urlBuf[640];
  HTTPClient http;
  http.setConnectTimeout(2500);
  http.setTimeout(2500);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  buildOwmUrl(urlBuf, sizeof(urlBuf), "weather", cityQ, owmUseCountry);
  if (!http.begin(urlBuf, OPENWEATHER_ROOT_CA)) {
    next.weatherHttp = HTTPC_ERROR_CONNECTION_REFUSED; next.ok = false;
    publishWeatherSnapshot(next); return;
  }
  int httpCode = http.GET();
  if (httpCode == 404 && owmUseCountry) {
    http.end(); owmUseCountry = false;
    buildOwmUrl(urlBuf, sizeof(urlBuf), "weather", cityQ, false);
    if (!http.begin(urlBuf, OPENWEATHER_ROOT_CA)) {
      next.weatherHttp = HTTPC_ERROR_CONNECTION_REFUSED; next.ok = false;
      publishWeatherSnapshot(next); return;
    }
    httpCode = http.GET();
  }
  next.weatherHttp = httpCode;
  bool weatherParsed = false;
  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, http.getStream());
    if (!err && doc["main"]["temp"].is<float>() && doc["main"]["humidity"].is<int>()) {
      next.temperature = doc["main"]["temp"].as<float>();
      next.feelsLike = doc["main"]["feels_like"].as<float>();
      next.humidity = doc["main"]["humidity"].as<int>();
      const char* main = doc["weather"][0]["main"];
      const char* desc = doc["weather"][0]["description"];
      if (main && desc) {
        strlcpy(next.main, main, sizeof(next.main));
        strlcpy(next.description, desc, sizeof(next.description));
        if (next.description[0]) next.description[0] = toupper((unsigned char)next.description[0]);
        next.ok = true; weatherParsed = true;
      }
    }
  }
  http.end();
  if (!weatherParsed) {
    next.ok = false;
    publishWeatherSnapshot(next);
    return;
  }

  if (ESP.getFreeHeap() < 50000) {
    next.forecastHttp = HTTPC_ERROR_TOO_LESS_RAM;
    publishWeatherSnapshot(next);
    return;
  }
  buildOwmUrl(urlBuf, sizeof(urlBuf), "forecast", cityQ, owmUseCountry);
  if (!http.begin(urlBuf, OPENWEATHER_ROOT_CA)) {
    next.forecastHttp = HTTPC_ERROR_CONNECTION_REFUSED;
    publishWeatherSnapshot(next); return;
  }
  httpCode = http.GET();
  next.forecastHttp = httpCode;
  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument fdoc(2048);
    DeserializationError err = deserializeJson(fdoc, http.getStream());
    if (!err) {
      struct tm tmNow; currentTm(&tmNow);
      const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
      const int indices[3] = { 8, 16, 24 };
      JsonArray list = fdoc["list"].as<JsonArray>();
      for (int i = 0; i < 3; ++i) {
        if ((size_t)indices[i] >= list.size()) continue;
        JsonObject item = list[indices[i]];
        if (!item["main"]["temp"].is<float>()) continue;
        next.forecast[i].temp = (int)item["main"]["temp"].as<float>();
        const char* icon = item["weather"][0]["main"];
        if (icon) strlcpy(next.forecast[i].iconType, icon, sizeof(next.forecast[i].iconType));
        strlcpy(next.forecast[i].dayName, days[(tmNow.tm_wday + i + 1) % 7], sizeof(next.forecast[i].dayName));
      }
    }
  }
  http.end();
  publishWeatherSnapshot(next);
}
void wifiTick(unsigned long now) {
  if (wifiWant && !wifiOn && now >= wifiRetryAt) { wifiRetryAt = now + 30000; Serial.println("WiFi: retrying..."); WiFi.begin(wifiSsid.c_str(), wifiPass.c_str()); }
  if (wifiWant && !wifiOn && WiFi.status() == WL_CONNECTED) {
    wifiOn = true; configTime(0, 0, ntpServer); setenv("TZ", tzString.c_str(), 1); tzset(); timeSynced = false;
    if (!telemetryServerStarted) {
      const char* headerNames[] = { "X-DeskPal-Token" };
      configServer.collectHeaders(headerNames, 1);
      configServer.on("/api/telemetry", HTTP_GET, sendTelemetryApi);
      configServer.on("/api/telemetry", HTTP_OPTIONS, handleCorsOptions);
      configServer.onNotFound([]() {
        configServer.sendHeader("Access-Control-Allow-Origin", "*");
        configServer.send(404, "application/json", "{\"error\":\"not found\"}");
      });
      MDNS.begin("deskpal");
      configServer.begin(); telemetryServerStarted = true;
      Serial.print("Telemetry API: http://"); Serial.print(WiFi.localIP()); Serial.println("/api/telemetry");
      Serial.print("Telemetry token: "); Serial.println(telemetryToken);
    }
    lastWeatherUpdate = now;
    Serial.println("WiFi: connected; waiting for NTP");
  }
  if (wifiOn && WiFi.status() != WL_CONNECTED) { wifiOn = false; wifiRetryAt = now + 10000; Serial.println("WiFi: lost, will retry"); }
  if (wifiOn && !timeSynced) {
    time_t epoch = 0; time(&epoch);
    if (epoch >= 1700000000) { timeSynced = true; lastWeatherUpdate = 0; Serial.println("NTP time synchronized"); }
  }
}
#endif


// --- End DeskPalNetwork.h ---



bool currentTm(struct tm* t) {
  if (timeSynced) {
    time_t epoch; time(&epoch);
    return localtime_r(&epoch, t) != nullptr;
  }
  time_t demo = demoBaseEpoch + millis() / 1000;
  return gmtime_r(&demo, t) != nullptr;
}


// --- Inlined from DeskPalDisplay.h ---

// OLED drawing primitives, pages, and boot animation.
void drawTypewriterText(const char* fullText, int yStart, int maxLines, unsigned long startTime) {
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  display.fillRect(0, yStart, SCREEN_WIDTH, maxLines * 10 + 2, OLED_BLACK);
  int totalLen = strlen(fullText);
  int vis = (int)((millis() - startTime) / 30);
  int shown = min(totalLen, vis);
  char s[96]; strlcpy(s, fullText, sizeof(s));
  if (shown < (int)sizeof(s) - 1) s[shown] = '\0';
  int y = yStart, line = 0;
  char cur[21]; int curLen = 0;
  int sLen = strlen(s);
  for (int i = 0; i <= sLen; i++) {
    char ch = (i < sLen) ? s[i] : '\0';
    if (ch == ' ' && curLen == 0) continue;
    if (ch == '\0' || ch == ' ' || curLen == 20) {
      if (curLen) {
        cur[curLen] = '\0'; display.fillRect(0, y, SCREEN_WIDTH, 10, OLED_BLACK); display.setCursor(2, y); display.print(cur);
        curLen = 0; y += 10; line++; if (line == maxLines) break;
      }
      if (ch == ' ') continue;
      if (ch == '\0') break;
    }
    cur[curLen++] = ch;
  }
  if (line < maxLines && curLen > 0) { cur[curLen] = '\0'; display.fillRect(0, y, SCREEN_WIDTH, 10, OLED_BLACK); display.setCursor(2, y); display.print(cur); }
}
void drawFitLine(const char* text, int y, int maxChars = 21) {
  char line[24];
  strlcpy(line, text, sizeof(line));
  if ((int)strlen(line) > maxChars) line[maxChars] = '\0';
  display.setFont(NULL);
  display.setTextColor(OLED_WHITE);
  display.setCursor(2, y);
  display.print(line);
}
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 16, OLED_WHITE); display.setFont(NULL); display.setTextColor(OLED_BLACK);
  display.setCursor(64 - (int)(strlen(title) * 3), 4); display.print(title); display.setTextColor(OLED_WHITE);
}
void drawWarnIcon(int x, int y) {
  x = ci(x, 0, SCREEN_WIDTH - 15); y = ci(y, 0, SCREEN_HEIGHT - 13);
  display.fillTriangle(x + 7, y, x, y + 12, x + 14, y + 12, OLED_WHITE);
  display.drawLine(x + 7, y + 3, x + 7, y + 8, OLED_BLACK); display.drawPixel(x + 7, y + 10, OLED_BLACK);
}
void drawBatteryIcon(int x, int y) {
  if (isnan(batteryPct)) return;
  display.drawRect(x, y + 2, 18, 9, OLED_WHITE); display.fillRect(x + 18, y + 4, 2, 5, OLED_WHITE);
  int segs = ci((int)round(batteryPct / 25.0f), 0, 4);
  for (int i = 0; i < segs; i++) display.fillRect(x + 2 + i * 4, y + 4, 3, 5, OLED_WHITE);
}
void drawWifiIcon(int x, int y, bool on) {
  if (!on) { display.setFont(NULL); display.setCursor(x, y); display.print("x"); return; }
  for (int r = 2; r <= 6; r += 2) drawFatArc(x + 4, y + 7, r, 220, 320, 1, x, y, 10, 10);
}
void drawEmooEye(Eye& e, bool isLeft, int mood) {
  clampEyeState(e);
  int ix = (int)e.x, iy = (int)e.y, iw = (int)e.w, ih = (int)e.h;
  ix = ci(ix, 0, SCREEN_WIDTH - 1); iy = ci(iy, 0, SCREEN_HEIGHT - 1);
  iw = ci(iw, 4, SCREEN_WIDTH - ix); ih = ci(ih, 4, SCREEN_HEIGHT - iy);
  int cx = ix + iw / 2, cy = iy + ih / 2;
  switch (mood) {
    case MOOD_HAPPY: drawFatArc(cx, cy + ih / 4, ci(iw / 2, 4, 20), 200, 340, 6, ix, iy, iw, ih); break;
    case MOOD_EXCITED:
      if (isLeft) { drawFatLine(ix + 2, iy + 2, cx, cy, 5, ix, iy, iw, ih); drawFatLine(ix + 2, iy + ih - 3, cx, cy, 5, ix, iy, iw, ih); }
      else { drawFatLine(ix + iw - 3, iy + 2, cx, cy, 5, ix, iy, iw, ih); drawFatLine(ix + iw - 3, iy + ih - 3, cx, cy, 5, ix, iy, iw, ih); }
      break;
    case MOOD_SLEEPY: display.fillRoundRect(ix + 2, cy - 2, iw - 4, 3, 2, OLED_WHITE); display.fillRoundRect(ix + 2, cy + 2, iw - 4, 3, 2, OLED_WHITE); break;
    case MOOD_COLD:
      { int ey = iy + ih / 4, eh = ih / 2;
        display.fillRoundRect(ix + 2, ey, iw - 4, eh, 4, OLED_WHITE);
        for (int k = 0; k < 3; k++) { int x = ix + 4 + k * ((iw - 8) / 2); display.fillTriangle(x, ey + eh - 1, x + 3, ey + eh - 1, x + 1, ey + eh + 4, OLED_WHITE); } }
      break;
    case MOOD_SCARED:
      if (isLeft) drawFatLine(cx - iw / 2 + 3, cy + ih / 2 - 3, cx + iw / 2 - 3, cy - ih / 2 + 3, 8, ix, iy, iw, ih);
      else drawFatLine(cx - iw / 2 + 3, cy - ih / 2 + 3, cx + iw / 2 - 3, cy + ih / 2 - 3, 8, ix, iy, iw, ih);
      break;
    case MOOD_SAD: drawFatArc(cx, cy - ih / 4, ci(iw / 2, 4, 20), 20, 160, 6, ix, iy, iw, ih); break;
    default: {
      int r = (iw < 20) ? 3 : 8;
      display.fillRoundRect(ix, iy, iw, ih, r, OLED_WHITE);
      int pw = ci((int)((iw / 2.2f) * pupilScale), 4, iw - 2);
      int ph = ci((int)((ih / 2.2f) * pupilScale), 4, ih - 2);
      int px = ci(cx + (int)e.pupilX - pw / 2, ix, ix + iw - pw);
      int py = ci(cy + (int)e.pupilY - ph / 2, iy, iy + ih - ph);
      display.fillRoundRect(px, py, pw, ph, r / 2, OLED_BLACK);
      if (iw > 15 && ih > 15 && pupilScale > 0.7f) drawClippedPixel(px + pw - 4, py + 4, ix, iy, iw, ih, OLED_WHITE);
      if (mood == MOOD_ANGRY) {
        for (int i = 0; i < ih / 2; i++) {
          int y = iy + i;
          int x0 = isLeft ? ix + (i * iw / (ih / 2 + 1)) : ix;
          int x1 = isLeft ? ix + iw : ix + iw - (i * iw / (ih / 2 + 1));
          drawClippedLine(x0, y, x1, y, ix, iy, iw, ih, OLED_BLACK);
        }
      }
      break;
    }
  }
}
void applyGesture(unsigned long now) {
  float gx = 0, gy = 0, gpx = 0, gpy = 0;
  switch (currentMood) {
    case MOOD_HAPPY: gy = sinf(now / 250.0f) * 2.0f; break;
    case MOOD_EXCITED: gy = sinf(now / 140.0f) * 3.0f; break;
    case MOOD_LOVE: gy = sinf(now / 500.0f) * 1.5f; gpx = sinf(now / 700.0f); break;
    case MOOD_ANGRY: gx = ((now / 80) % 2) ? 1.5f : -1.5f; break;
    case MOOD_SCARED: gx = random(-1, 2); gy = random(-1, 2); break;
    case MOOD_SAD: gpy = 4; gy = 1.5f; break;
    case MOOD_SUSPICIOUS: gpx = 6; break;
    case MOOD_SLEEPY: gy = 2 + sinf(now / 900.0f); break;
    case MOOD_SURPRISED: gy = random(-1, 2); break;
    case MOOD_COLD: gx = sinf(now / 60.0f); gy = random(-1, 2) * 0.5f; break;
  }
  leftEye.targetX += gx; leftEye.targetY += gy;
  rightEye.targetX += gx; rightEye.targetY += gy;
  leftEye.targetPupilX += gpx; leftEye.targetPupilY += gpy;
  rightEye.targetPupilX += gpx; rightEye.targetPupilY += gpy;
  clampEyeTargets(leftEye); clampEyeTargets(rightEye);
}
void updatePhysicsAndMood() {
  unsigned long now = millis();
  breathVal = sinf(now / 800.0f) * 1.5f;
  if (now > leftEye.nextBlinkTime && currentMood != MOOD_SLEEPY) {
    leftEye.blinking = true; leftEye.lastBlink = now; leftEye.nextBlinkTime = now + random(2000, 6000);
  }
  if (leftEye.blinking) {
    leftEye.targetH = 2; rightEye.targetH = 2;
    if (now - leftEye.lastBlink > 120) leftEye.blinking = false;
  }
  if (!leftEye.blinking && now - lastSaccade > saccadeInterval) {
    lastSaccade = now; saccadeInterval = random(500, 3000);
    int dir = random(0, 10); float lx = 0, ly = 0;
    if (dir == 4) { lx = -6; ly = -4; } else if (dir == 5) { lx = 6; ly = -4; }
    else if (dir == 6) { lx = -6; ly = 4; } else if (dir == 7) { lx = 6; ly = 4; }
    else if (dir == 8) { lx = 8; ly = 0; } else if (dir == 9) { lx = -8; ly = 0; }
    leftEye.targetPupilX = lx; leftEye.targetPupilY = ly;
    rightEye.targetPupilX = lx; rightEye.targetPupilY = ly;
    leftEye.targetX = 18 + lx * 0.3f; leftEye.targetY = 14 + ly * 0.3f;
    rightEye.targetX = 74 + lx * 0.3f; rightEye.targetY = 14 + ly * 0.3f;
  }
  if (!leftEye.blinking) {
    float baseW = 36, baseH = 36 + breathVal;
    switch (currentMood) {
      case MOOD_HAPPY: case MOOD_LOVE: baseW = 40; baseH = 32; break;
      case MOOD_SURPRISED: baseW = 30; baseH = 45; break;
      case MOOD_SLEEPY: baseW = 38; baseH = 30; break;
      case MOOD_ANGRY: baseW = 34; baseH = 32; break;
      case MOOD_SAD: baseW = 34; baseH = 30; leftEye.targetY = 30; rightEye.targetY = 30; break;
      case MOOD_SCARED: baseW = 30; baseH = 22; break;
      case MOOD_EXCITED: baseW = 36; baseH = 34; leftEye.targetX = 26; rightEye.targetX = 66; break;
      case MOOD_COLD: baseW = 32; baseH = 30; break;
      case MOOD_SUSPICIOUS:
        leftEye.targetW = 36; leftEye.targetH = 20; rightEye.targetW = 36; rightEye.targetH = 42;
        applyGesture(now); leftEye.update(); rightEye.update(); clampEyeState(leftEye); clampEyeState(rightEye); return;
    }
    leftEye.targetW = baseW; leftEye.targetH = baseH; rightEye.targetW = baseW; rightEye.targetH = baseH;
  }
  applyGesture(now); leftEye.update(); rightEye.update(); clampEyeState(leftEye); clampEyeState(rightEye);
}
void drawEmoPage() {
  updatePhysicsAndMood(); unsigned long now = millis();
  if (currentMood == MOOD_LOVE) {
    int t1 = (now / 60) % 48; int t2 = ((now / 60) + 24) % 48;
    drawClippedBitmap(1, 48 - t1, bmp_heart, 16, 16, OLED_WHITE); drawClippedBitmap(111, 48 - t2, bmp_heart, 16, 16, OLED_WHITE);
  } else if (currentMood == MOOD_SLEEPY) drawClippedBitmap(110, 0, bmp_zzz, 16, 16, OLED_WHITE);
  else if (currentMood == MOOD_ANGRY) drawClippedBitmap(56, 0, bmp_anger, 16, 16, OLED_WHITE);
  else if (currentMood == MOOD_EXCITED) { drawClippedBitmap(4, 8, bmp_music, 8, 8, OLED_WHITE); drawClippedBitmap(116, 12, bmp_music, 8, 8, OLED_WHITE); }
  if (currentMood == MOOD_SCARED) {
    drawClippedFillCircle(64, 52, 9, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, OLED_WHITE);
    drawClippedFillCircle(64, 52, 6, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, OLED_BLACK);
    drawClippedFillCircle(64, 57, 3, 59, 47, 10, 10, OLED_WHITE);
  } else if (currentMood == MOOD_COLD) {
    display.fillRoundRect(56, 48, 16, 9, 3, OLED_WHITE);
    for (int x = 60; x <= 68; x += 4) display.drawLine(x, 48, x, 56, OLED_BLACK);
    display.drawLine(56, 52, 71, 52, OLED_BLACK);
    drawClippedBitmap(2, 2, bmp_snow, 8, 8, OLED_WHITE); drawClippedBitmap(114, 2, bmp_snow, 8, 8, OLED_WHITE);
    drawClippedBitmap(4, 44, bmp_snow, 8, 8, OLED_WHITE); drawClippedBitmap(112, 44, bmp_snow, 8, 8, OLED_WHITE);
  } else if (currentMood == MOOD_SUSPICIOUS) {
    drawFatArc(61, 50, 3, -70, 70, 2, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    drawFatArc(61, 56, 3, -70, 70, 2, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    drawClippedBitmap(118, 0, bmp_tiny_drop, 8, 8, OLED_WHITE);
  }
  if (anomalyActive) drawWarnIcon(0, 0);
  if (quietMode) { display.setFont(NULL); display.setCursor(120, 0); display.print("M"); }
  if (micMuted) { display.setFont(NULL); display.setCursor(108, 0); display.print("X"); }
  drawEmooEye(leftEye, true, currentMood); drawEmooEye(rightEye, false, currentMood);
}
void drawClock() {
  struct tm t; currentTm(&t);
  drawHeader(timeSynced ? "CLOCK" : "CLOCK (DEMO)");
  display.setTextColor(OLED_WHITE); display.setFont(NULL); display.setTextSize(1);
  display.setFont(&FreeSansBold18pt7b);
  char timeStr[6]; sprintf(timeStr, "%02d:%02d", t.tm_hour, t.tm_min);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(max(0, (SCREEN_WIDTH - (int)w) / 2), 43); display.print(timeStr);
  display.setFont(NULL);
  char dateStr[20]; strftime(dateStr, 20, "%a, %b %d", &t);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(max(0, (SCREEN_WIDTH - (int)w) / 2), 57); display.print(dateStr);
  drawWifiIcon(116, 2, wifiOn);
  drawBatteryIcon(94, 1);
}
void drawEnvPage() {
  drawHeader("ENVIRONMENT"); display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (!sensorsOk) {
    drawFitLine(sensorFailCount > 0 && sensorFailCount < SENSOR_RECOVERY_FAILS ? "Sensor recovering..." : "No sensor data", 29);
    drawFitLine("Check BME280 wiring", 42);
    return;
  }
  char line[32];
  snprintf(line, sizeof(line), "Room: %.1f C  RH:%d%%", envTemp, (int)envHum); drawFitLine(line, 21);
#if ENABLE_WEATHER
  WeatherSnapshot weather = getWeatherSnapshot();
  if (wifiOn && weather.ok) {
    snprintf(line, sizeof(line), "Outside: %.1f C", weather.temperature); drawFitLine(line, 31);
    snprintf(line, sizeof(line), "Room vs out: %+.1f C", envTemp - weather.temperature); drawFitLine(line, 41);
  } else {
    drawFitLine("Outside: not available", 31);
    snprintf(line, sizeof(line), "Pressure: %dhPa", (int)envPress); drawFitLine(line, 41);
  }
#else
  snprintf(line, sizeof(line), "Pressure: %dhPa", (int)envPress); drawFitLine(line, 31);
  snprintf(line, sizeof(line), "Altitude: %dm", (int)envAlt); drawFitLine(line, 41);
#endif
  snprintf(line, sizeof(line), "Comfort: %s", comfortNames[comfortClass]); drawFitLine(line, 52);
}
void drawInsightsPage() {
  drawHeader("INSIGHTS");
  if (insightCount == 0) { display.setFont(NULL); display.setCursor(10, 30); display.print("Thinking..."); return; }
  int idx = (millis() / 8000) % insightCount;
  static int shownIdx = -1; static unsigned long typeStart = 0;
  if (idx != shownIdx) { shownIdx = idx; typeStart = millis(); }
  drawTypewriterText(insights[idx], 18, 4, typeStart);
  display.setFont(NULL); display.setCursor(112, 56);
  char cntBuf[16]; snprintf(cntBuf, sizeof(cntBuf), "%d/%d", idx + 1, insightCount); display.print(cntBuf);
}
void drawKwsPage() {
#if USE_TFLITE
  drawHeader("VOICE MODEL");
#else
  drawHeader("MIC LEVEL");
#endif
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (isAiResponding && millis() - aiResponseStart < 4000) { drawTypewriterText(aiResponse, 20, 3, aiResponseStart); }
  else {
    isAiResponding = false;
    bool kws = (millis() < kwsActiveUntil) || isListeningForCommand;
    for (int i = 0; i < 16; i++) {
      float target;
      if (kws) target = 24.0f;
      else if (micMuted) target = 2.0f;
      else if (micReady) target = cf(3.0f + micRms / 150.0f, 2.0f, 24.0f);
      else target = 2.0f;
      kwsBars[i] = kwsBars[i] * 0.6f + target * 0.4f;
      int hgt = ci((int)kwsBars[i], 0, 24);
      display.fillRect(i * 8 + 1, 44 - hgt, 5, hgt, OLED_WHITE);
    }
    display.drawLine(0, 45, 128, 45, OLED_WHITE);
    if (!micReady) { display.setCursor(4, 54); display.print("Mic unavailable"); }
    else if (micMuted) { display.setCursor(4, 54); display.print("Mic muted"); }
    else if (kws) { display.setFont(&FreeSansBold9pt7b); display.setCursor(28, 56); display.print("LISTENING!"); }
    else if (KWS_MODEL_INTEGRATED) { display.setCursor(4, 54); display.print("Say \"Hey Pal\"!"); }
    else drawFitLine("Level only; no speech model", 54);
  }
}
#if ENABLE_WEATHER
void drawWeatherCard() {
  drawHeader("WEATHER");
  WeatherSnapshot weather = getWeatherSnapshot();
  if (!wifiOn) { drawFitLine("WiFi not connected", 27); drawFitLine("Setup: hold touch at boot", 39); return; }
  if (city.length() == 0 || apiKey.length() == 0) { drawFitLine("Set city + API key", 27); drawFitLine("in setup page", 39); return; }
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (!weather.ok) {
    drawFitLine(cityUpper, 23);
    char eb[32];
    if (weather.weatherHttp == 401) snprintf(eb, sizeof(eb), "401 bad API key");
    else if (weather.weatherHttp == 404) snprintf(eb, sizeof(eb), "404 city not found");
    else if (weather.weatherHttp == 429) snprintf(eb, sizeof(eb), "429 call limit");
    else if (weather.weatherHttp != 0) snprintf(eb, sizeof(eb), "HTTP %d failed", weather.weatherHttp);
    else snprintf(eb, sizeof(eb), "First fetch pending");
    drawFitLine(eb, 36);
    drawFitLine("Retry in about 60 s", 48);
    drawFitLine("See USB serial log", 58);
    return;
  }
  display.drawBitmap(96, 19, getMiniIcon(weather.main), 16, 16, OLED_WHITE);
  drawFitLine(cityUpper, 22, 15);
  char line[32];
  snprintf(line, sizeof(line), "Outside: %.1f C", weather.temperature); drawFitLine(line, 34);
  snprintf(line, sizeof(line), "Feels: %.1f C RH:%d%%", weather.feelsLike, weather.humidity); drawFitLine(line, 45);
  drawFitLine(weather.description, 55);
}
void drawForecastPage() {
  drawHeader("3-DAY FORECAST"); display.setTextColor(OLED_WHITE);
  WeatherSnapshot weather = getWeatherSnapshot();
  if (!wifiOn) { display.setFont(NULL); display.setCursor(20, 30); display.print("No WiFi yet"); return; }
  if (weather.forecast[0].dayName[0] == '\0') {
    display.setFont(NULL);
    char eb[32];
    if (weather.forecastHttp != 0 && weather.forecastHttp != 200) snprintf(eb, sizeof(eb), "Forecast HTTP %d", weather.forecastHttp);
    else snprintf(eb, sizeof(eb), "Waiting for forecast");
    display.setCursor(20, 30); display.print(eb);
    display.setCursor(20, 42); display.print("Retry in 60 s");
    return;
  }
  display.drawLine(42, 16, 42, 63, OLED_WHITE); display.drawLine(85, 16, 85, 63, OLED_WHITE);
  for (int i = 0; i < 3; i++) {
    int centerX = i * 43 + 21;
    display.setFont(NULL);
    const char* d = weather.forecast[i].dayName;
    display.setCursor(centerX - (int)(strlen(d) * 3), 20); display.print(d);
    display.drawBitmap(centerX - 8, 28, getMiniIcon(weather.forecast[i].iconType), 16, 16, OLED_WHITE);
    char tBuf[8]; snprintf(tBuf, sizeof(tBuf), "%d C", weather.forecast[i].temp);
    int16_t x1, y1; uint16_t w, h;
    display.setFont(NULL);
    display.getTextBounds(tBuf, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(centerX - (int)(w / 2), 56); display.print(tBuf);
  }
}
#endif
void drawWorldClock() {
  time_t now;
  if (timeSynced) time(&now); else now = demoBaseEpoch + millis() / 1000;
  time_t indEpoch = now + 5 * 3600 + 30 * 60;
  time_t sydEpoch = now + 11 * 3600;
  struct tm* it = gmtime(&indEpoch); int i_h = it->tm_hour, i_m = it->tm_min;
  struct tm* st = gmtime(&sydEpoch); int s_h = st->tm_hour, s_m = st->tm_min;
  drawHeader("WORLD CLOCK"); display.drawLine(64, 18, 64, 59, OLED_WHITE);
  char iStr[10], sStr[10]; snprintf(iStr, sizeof(iStr), "%02d:%02d", i_h, i_m); snprintf(sStr, sizeof(sStr), "%02d:%02d", s_h, s_m);
  display.setFont(NULL); display.setCursor(14, 25); display.print("INDIA"); display.setCursor(5, 43); display.print(iStr);
  display.setCursor(75, 25); display.print("SYDNEY"); display.setCursor(68, 43); display.print(sStr);
  display.setCursor(9, 56); display.print("Button: next page");
}

void drawTimerPage() {
  drawHeader(timerRunning ? "STUDY TIMER" : (isTimerAlarm ? "TIME'S UP!" : "SET TIMER"));
  int mins = timerRemainingSec / 60, secs = timerRemainingSec % 60;
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mins, secs);
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (isTimerAlarm) {
    display.setCursor(4, 29); display.print("Timer complete!");
    display.setCursor(4, 43); display.print("Press button to stop");
  }
  else if (!timerRunning) {
    display.setCursor(4, 20); display.print("Duration (minutes)");
    display.setFont(&FreeSansBold9pt7b);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
    int cx = (SCREEN_WIDTH - (int)w) / 2;
    display.setCursor(cx, 44); display.print(timeBuf);
    display.setFont(NULL);
    display.setCursor(2, 47); display.print("Touch+5 Hold:reset");
    display.setCursor(2, 57); display.print("Btn:start Hold:exit");
  } else {
    display.setCursor(2, 20);
    if (timerRemainingSec < 60) display.print("ALMOST DONE!");
    else display.print("FOCUSING...");
    display.setFont(&FreeSansBold9pt7b);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
    int cx = (SCREEN_WIDTH - (int)w) / 2;
    display.setCursor(cx, 44); display.print(timeBuf);
    display.setFont(NULL);
    display.drawRect(14, 48, 100, 6, OLED_WHITE);
    if (timerDurationSec > 0) {
      int fill = (int)((float)(timerDurationSec - timerRemainingSec) / timerDurationSec * 96);
      if (fill < 0) fill = 0;
      display.fillRect(16, 49, fill, 4, OLED_WHITE);
    }
    display.setCursor(2, 57); display.print("Btn:pause Hold:exit");
  }
}

void drawTimerOverlay() {
  if (!timerRunning || currentPage == PAGE_TIMER) return;
  int mins = timerRemainingSec / 60, secs = timerRemainingSec % 60;
  char buf[8]; snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
  int w = 36, h = 12;
  int x = SCREEN_WIDTH - w - 2, y = 2;
  display.fillRect(x, y, w, h, OLED_BLACK); display.drawRect(x, y, w, h, OLED_WHITE);
  display.setFont(NULL); display.setTextColor(OLED_WHITE); display.setCursor(x + 3, y + 2); display.print(buf);
}

void playBootAnimation() {
  display.setTextColor(OLED_WHITE); int cx = 64, cy = 32;
  for (int r = 0; r < 80; r += 4) { display.clearDisplay(); display.fillCircle(cx, cy, r, OLED_WHITE); display.display();  delay(10); }
  for (int r = 0; r < 80; r += 4) { display.clearDisplay(); display.fillCircle(cx, cy, 80, OLED_WHITE); display.fillCircle(cx, cy, r, OLED_BLACK); display.display();  delay(10); }
  display.setFont(&FreeSansBold9pt7b); const char* bootText = "DeskPal";
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(bootText, 0, 0, &x1, &y1, &w, &h);
  display.clearDisplay(); display.setCursor((SCREEN_WIDTH - (int)w) / 2, 36); display.print(bootText); display.display();
  unsigned long start = millis();
  while (millis() - start < 1200) {  delay(20); }
}


// --- End DeskPalDisplay.h ---


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
  if (OLED_TIMEOUT_MS > 0 && !screenOff && now - lastInteraction > OLED_TIMEOUT_MS) { DISPLAY_OFF(); screenOff = true; }
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
