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
#include "comfort_learner.h"

#if defined(__has_include)
#if __has_include("DeskPalSecrets.h")
#include "DeskPalSecrets.h"
#endif
#endif
#ifndef DESKPAL_WEATHER_API_KEY
#define DESKPAL_WEATHER_API_KEY ""
#endif
#ifndef DESKPAL_WIFI_SSID
#define DESKPAL_WIFI_SSID ""
#endif
#ifndef DESKPAL_WIFI_PASS
#define DESKPAL_WIFI_PASS ""
#endif

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
// CORS "*" is for local prototype testing only; set this to the dashboard origin when hosted.
#ifndef DESKPAL_CORS_ORIGIN
#define DESKPAL_CORS_ORIGIN "*"
#endif
#if ENABLE_WEATHER
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#endif

#include <Adafruit_BME280.h>

const unsigned char bmp_clear[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0xc0,0x80,0x00,0x0f,0xf0,0x00,0x00,0x3f,0xfc,0x00,0x00,0x7f,0xfe,0x00,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x3e,0xff,0xff,0x7c,0x00,0xff,0xff,0x00,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x06,0xff,0xff,0x60,0x00,0xff,0xff,0x00,0x00,0x7f,0xfe,0x00,0x00,0x3f,0xfc,0x00,0x01,0x0f,0xf0,0x80,0x00,0x03,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_clouds[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x3f,0xff,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x00,0xff,0xff,0xe0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_rain[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x00,0x00,0x0f,0xf8,0x00,0x00,0x1f,0xfc,0x00,0x00,0x3f,0xfe,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff,0xc0,0x01,0xff,0xff,0xf0,0x03,0xff,0xff,0xf8,0x07,0xff,0xff,0xfc,0x0f,0xff,0xff,0xfe,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x1f,0xff,0xff,0xff,0x0f,0xff,0xff,0xfe,0x07,0xff,0xff,0xfc,0x03,0xff,0xff,0xf8,0x00,0xff,0xff,0xe0,0x00,0x3f,0xff,0x80,0x00,0x0f,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x0c,0x00,0x00,0x60,0x0c,0x00,0x00,0xe0,0x1c,0x00,0x00,0xc0,0x18,0x00,0x03,0x80,0x70,0x00,0x03,0x80,0x70,0x00,0x03,0x00,0x60,0x00,0x02,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char mini_sun[] PROGMEM = {0x00,0x00,0x01,0x80,0x00,0x00,0x10,0x08,0x04,0x20,0x03,0xc0,0x27,0xe4,0x07,0xe0,0x07,0xe0,0x27,0xe4,0x03,0xc0,0x04,0x20,0x10,0x08,0x00,0x00,0x01,0x80,0x00,0x00};
const unsigned char mini_cloud[] PROGMEM = {0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x0f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char mini_rain[] PROGMEM = {0x00,0x00,0x00,0x00,0x01,0xc0,0x07,0xe0,0x0f,0xf0,0x1f,0xf8,0x1f,0xf8,0x3f,0xfc,0x3f,0xfc,0x7f,0xfe,0x3f,0xfe,0x1f,0xfc,0x00,0x00,0x44,0x44,0x22,0x22,0x11,0x11};
const unsigned char bmp_tiny_drop[] PROGMEM = {0x10,0x38,0x7c,0xfe,0xfe,0x7c,0x38,0x00};
const unsigned char bmp_heart[] PROGMEM = {0x00,0x00,0x0c,0x60,0x1e,0xf0,0x3f,0xf8,0x7f,0xfc,0x7f,0xfc,0x7f,0xfc,0x3f,0xf8,0x1f,0xf0,0x0f,0xe0,0x07,0xc0,0x03,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_zzz[] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,0x30,0x00,0x7e,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,0x30,0x00,0x7c,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_anger[] PROGMEM = {0x00,0x00,0x11,0x10,0x2a,0x90,0x44,0x40,0x80,0x20,0x80,0x20,0x44,0x40,0x2a,0x90,0x11,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_music[] PROGMEM = {0x01,0x80,0x01,0x80,0x01,0x80,0x01,0xc0,0x01,0xe0,0x03,0xf0,0x07,0xe0,0x0f,0xc0,0x07,0x80,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char bmp_snow[] PROGMEM = {0x99,0x5A,0x3C,0xFF,0xFF,0x3C,0x5A,0x99};

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
#define DESKPAL_KWS_ADAPTER_READY 0
int classifyKeywordEdge();
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
#define OLED_TIMEOUT_MS          0UL
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
#define PAGE_WEATHER 2
#define PAGE_FORECAST 3
#define PAGE_ENV 4
#define PAGE_INSIGHTS 5
#define PAGE_KWS 6
#define PAGE_TIMER 7

#define SENSOR_GRACE_MS 8000UL
#define SENSOR_RECOVERY_FAILS 5
#define ALERT_DEFAULT_TMAX_C 30.0f
#define ALERT_DEFAULT_TMIN_C 15.0f
#define ALERT_DEFAULT_HMAX_PCT 80.0f
#define ALERT_REPEAT_MS 600000UL
#define COMFORT_OK 0
#define COMFORT_HOT 1
#define COMFORT_COLD 2
#define COMFORT_HUMID 3
#define COMFORT_DRY 4

#define CONFIG_HOLD_MS 3000
#define CONFIG_AP_PIN_LENGTH 8

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

struct ClimateParams { float tMid, tHalf, hMid, hHalf; };
struct ForecastDay { char dayName[4]; int temp; char iconType[16]; };
struct BuzzerStep { uint16_t onMs; uint16_t offMs; };
#if ENABLE_WEATHER
struct WeatherSnapshot {
  bool ok;
  int weatherHttp, forecastHttp;
  long timezoneOffset;
  float temperature, feelsLike;
  int humidity;
  char main[16], description[32];
  ForecastDay forecast[3];
};
#endif

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
WiFiClient mqttNet;
PubSubClient mqtt(mqttNet);
WeatherSnapshot weatherState = {};
portMUX_TYPE weatherStateMux = portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t weatherTaskHandle = nullptr;
volatile bool weatherFetchInFlight = false;
#endif

Eye leftEye, rightEye;
bool micReady = false;
bool currentTm(struct tm* t);

#if ENABLE_WEATHER
void sendTelemetryApi();
void sendEventsApi();
void handleWifiSaveApi();
void handleTimerApi();
void handleCalibrationApi();
void handleAlertsApi();
void handleComfortApi();
void mqttComfortCommand(char* topic, byte* payload, unsigned int length);
void publishComfortTelemetry();
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

float envTemp = NAN, envTempRaw = NAN, envHum = NAN, envPress = NAN, envAlt = NAN;
float tempOffsetC = TEMP_OFFSET;
float alertTmax = ALERT_DEFAULT_TMAX_C;
float alertTmin = ALERT_DEFAULT_TMIN_C;
float alertHmax = ALERT_DEFAULT_HMAX_PCT;
ComfortLearner temperatureComfort(1.2f, 2.0f, 12.0f, 30.0f, 18.0f, 38.0f, 2.0f);
ComfortLearner humidityComfort(0.15f, 12.0f, 15.0f, 60.0f, 50.0f, 95.0f, 15.0f);
static const size_t COMFORT_BLOB_SIZE = ComfortLearner::SERIALIZED_SIZE * 2U;
bool comfortLearnersReady = false;
bool hasLastComfortMutation = false;
unsigned long lastComfortMutationAt = 0;
bool lastComfortTempUpdated = false;
bool lastComfortHumUpdated = false;
unsigned long lastAlertTmaxAt = 0, lastAlertTminAt = 0, lastAlertHmaxAt = 0;
bool alertedTmax = false, alertedTmin = false, alertedHmax = false;
bool sensorsOk = false, bmpOk = false;
int sensorFailCount = 0;
bool sensorUnavailableEventSent = false;
unsigned long lastGoodSensorRead = 0;
int comfortClass = COMFORT_OK;

// Dew point, heat index, comfort score, and mold risk are sensor-based estimates.
static const float DEW_POINT_A = 17.62f;
static const float DEW_POINT_B = 243.12f;
static const float HEAT_INDEX_MIN_TEMP_C = 27.0f;
static const float HEAT_INDEX_MIN_HUMIDITY = 40.0f;
static const float COMFORT_OPPRESSIVE_DEW_C = 21.0f;
static const unsigned long HUMIDITY_RISK_SUSTAINED_MS = 1800000UL;
float dewPointC = NAN;
float heatIndexC = NAN;
float comfortScore = NAN;
char moldRisk[8] = "LOW";
unsigned long humidityAbove70Since = 0, humidityAbove80Since = 0;
bool humidityAbove70Tracking = false, humidityAbove80Tracking = false;

static const uint8_t ANOMALY_WINDOW_SIZE = 20;
static const float ANOMALY_TEMP_Z_LIMIT = 2.5f;
static const float ANOMALY_HUM_Z_LIMIT = 2.5f;
static const float ANOMALY_PRESS_Z_LIMIT = 3.0f;
static const float ANOMALY_TEMP_SD_FLOOR = 0.2f;
static const float ANOMALY_HUM_SD_FLOOR = 1.0f;
static const float ANOMALY_PRESS_SD_FLOOR = 0.3f;
float anomalyTempWindow[ANOMALY_WINDOW_SIZE] = {};
float anomalyHumWindow[ANOMALY_WINDOW_SIZE] = {};
float anomalyPressWindow[ANOMALY_WINDOW_SIZE] = {};
uint8_t anomalyWindowIndex = 0;
uint8_t anomalyWindowCount = 0;
bool anomalyActive = false;
char anomalyMsg[48] = "";
unsigned long lastAnomalyNoticeAt = 0;
float pressHourAgo = NAN;
unsigned long lastPressSample = 0;
float lastInferenceMs = 0.0f;
float infAvg = 0.0f;

char insightTitle[32] = "ROOM CHECK";
char insightAction[48] = "Waiting for sensor data";
unsigned long lastSensorRead = 0;
unsigned long lastInsightGen = 0;

String city, countryCode, apiKey, wifiSsid, wifiPass, tzString;
char telemetryToken[33] = "";
char cityUpper[16] = "";
bool wifiOn = false;
bool wifiWant = false;
bool telemetryServerStarted = false;
unsigned long wifiRetryAt = 0;
#if ENABLE_WEATHER
enum WifiState { NET_WIFI_IDLE, NET_WIFI_CONNECTING, NET_WIFI_CONNECTED, NET_WIFI_BACKOFF };
enum MqttState { NET_MQTT_IDLE, NET_MQTT_CONNECTING, NET_MQTT_CONNECTED, NET_MQTT_BACKOFF };
WifiState wifiState = NET_WIFI_IDLE;
MqttState mqttState = NET_MQTT_IDLE;
uint8_t wifiFailureCount = 0;
uint8_t mqttFailureCount = 0;
unsigned long wifiAttemptStartedAt = 0;
void setWifiState(WifiState nextState, const char* message);
void setMqttState(MqttState nextState, const char* message);
void startWifiConnection(unsigned long now);
void handleWifiFailure(unsigned long now);
void networkRestart(const char* reason);
#endif
volatile bool timeSynced = false;
unsigned long lastWeatherUpdate = 0;
const char* ntpServer = "pool.ntp.org";
const time_t demoBaseEpoch = 1767258000;

int climateZone = CLIMATE_TEMPERATE;
float batteryPct = NAN;
float micRms = 0.0f;
float micNoiseFloor = 200.0f;

// Sampling runs every >2 s in loop(), with about 150 ms of sensor averaging.
// Keep one hour plus margin so pressure trends can use a genuine 1 h baseline.
static const uint16_t HISTORY_CAPACITY = 1800;
static const uint8_t TELEMETRY_HISTORY_COUNT = 60;
float histTemp[HISTORY_CAPACITY], histHum[HISTORY_CAPACITY], histPress[HISTORY_CAPACITY];
uint32_t histTimeMs[HISTORY_CAPACITY];
int histIdx = 0;
bool histFilled = false;
unsigned long trendDataStartAt = 0, lastTrendSampleAt = 0;
float tempTrendCPerHr = NAN, humTrendPerHr = NAN, pressTrendHpaPerHr = NAN;
char pressureForecast[24] = "Learning";
char temperaturePrediction[32] = "Temp estimate: --";
char inferred[32] = "None";
bool inferredWindowLatched = false, inferredCoolingLatched = false, inferredOccupancyLatched = false;
unsigned long inferredWindowAt = 0, inferredCoolingAt = 0, inferredOccupancyAt = 0;
static const unsigned long INFERENCE_COOLDOWN_MS = 600000UL;

struct DeviceEvent {
  uint32_t id;
  uint32_t uptimeMs;
  char source[16];
  char message[96];
};

int historySampleCount() { return histFilled ? HISTORY_CAPACITY : histIdx; }
int historyStartIndex(int count) {
  const int total = historySampleCount();
  if (count > total) count = total;
  return (histIdx + HISTORY_CAPACITY - count) % HISTORY_CAPACITY;
}
int historyIndex(int start, int offset) { return (start + offset) % HISTORY_CAPACITY; }
static const uint8_t DEVICE_EVENT_CAPACITY = 32;
DeviceEvent deviceEvents[DEVICE_EVENT_CAPACITY] = {};
uint32_t nextDeviceEventId = 1;
char pairCode[9] = "";
#if ENABLE_WEATHER
void mqttPublishEvent(const DeviceEvent& event);
#endif

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

int ci(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
float cf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void writeDeviceEvent(const DeviceEvent& event) {
  StaticJsonDocument<256> doc;
  doc["type"] = "decision";
  doc["id"] = event.id;
  doc["uptimeMs"] = event.uptimeMs;
  doc["source"] = event.source;
  doc["msg"] = event.message;
  serializeJson(doc, Serial);
  Serial.println();
}

void emitEvent(const char* source, const char* message) {
  uint32_t id = nextDeviceEventId++;
  DeviceEvent& event = deviceEvents[(id - 1) % DEVICE_EVENT_CAPACITY];
  event.id = id;
  event.uptimeMs = millis();
  strlcpy(event.source, source ? source : "System", sizeof(event.source));
  strlcpy(event.message, message ? message : "", sizeof(event.message));
  writeDeviceEvent(event);
#if ENABLE_WEATHER
  mqttPublishEvent(event);
#endif
}

void sendDeviceEventHistory(uint32_t afterId) {
  uint32_t oldestId = nextDeviceEventId > DEVICE_EVENT_CAPACITY ? nextDeviceEventId - DEVICE_EVENT_CAPACITY : 1;
  for (uint32_t id = oldestId; id < nextDeviceEventId; ++id) {
    if (id <= afterId) continue;
    const DeviceEvent& event = deviceEvents[(id - 1) % DEVICE_EVENT_CAPACITY];
    if (event.id == id) writeDeviceEvent(event);
  }
}

void emitDecision(const char* message) {
  const char* source = "System";
  if (strncmp(message, "MiniAI", 6) == 0) source = "Mini-AI";
  else if (strncmp(message, "Wake word", 9) == 0) source = "KWS";
  else if (strncmp(message, "Anomaly", 7) == 0 || strstr(message, "changed quickly")) source = "Anomaly";
  else if (strncmp(message, "Picked up", 9) == 0) source = "Lift";
  else if (strncmp(message, "Mic", 3) == 0) source = "Mic";
  else if (strncmp(message, "Over-petted", 11) == 0 || strncmp(message, "Long cuddle", 11) == 0) source = "Touch";
  else if (strncmp(message, "Timer", 5) == 0 || strncmp(message, "Voice command", 13) == 0) source = "Timer";
  emitEvent(source, message);
}

ClimateParams getDefaultClimateParams() {
  float tMin = 18.0f, tMax = 26.0f, hMin = 30.0f, hMax = 85.0f;
  if (climateZone == CLIMATE_COASTAL)      { tMin = 20.0f; tMax = 30.0f; hMin = 40.0f; hMax = 90.0f; }
  else if (climateZone == CLIMATE_DESERT)  { tMin = 18.0f; tMax = 29.0f; hMin = 15.0f; hMax = 65.0f; }
  else if (climateZone == CLIMATE_MOUNTAIN){ tMin = 14.0f; tMax = 24.0f; hMin = 25.0f; hMax = 80.0f; }
  ClimateParams z;
  z.tMid = (tMin + tMax) * 0.5f; z.tHalf = (tMax - tMin) * 0.5f;
  z.hMid = (hMin + hMax) * 0.5f; z.hHalf = (hMax - hMin) * 0.5f;
  return z;
}

ClimateParams getClimateParams() {
  ClimateParams z = getDefaultClimateParams();
  if (comfortLearnersReady && temperatureComfort.votes() >= 5) {
    z.tMid = (temperatureComfort.lo() + temperatureComfort.hi()) * 0.5f;
    z.tHalf = (temperatureComfort.hi() - temperatureComfort.lo()) * 0.5f;
  }
  if (comfortLearnersReady && humidityComfort.votes() >= 5) {
    z.hMid = (humidityComfort.lo() + humidityComfort.hi()) * 0.5f;
    z.hHalf = (humidityComfort.hi() - humidityComfort.lo()) * 0.5f;
  }
  return z;
}

void initializeComfortLearning() {
  const ClimateParams defaults = getDefaultClimateParams();
  const float defaultLoT = defaults.tMid - defaults.tHalf;
  const float defaultHiT = defaults.tMid + defaults.tHalf;
  const float defaultLoH = defaults.hMid - defaults.hHalf;
  const float defaultHiH = defaults.hMid + defaults.hHalf;
  temperatureComfort.init(defaultLoT, defaultHiT);
  humidityComfort.init(defaultLoH, defaultHiH);
  comfortLearnersReady = false;

  if (prefs.begin("deskpal", true)) {
    if (prefs.getBytesLength("comfort") == COMFORT_BLOB_SIZE) {
      unsigned char blob[COMFORT_BLOB_SIZE];
      ComfortLearner loadedTemperature(1.2f, 2.0f, 12.0f, 30.0f, 18.0f, 38.0f, 2.0f);
      ComfortLearner loadedHumidity(0.15f, 12.0f, 15.0f, 60.0f, 50.0f, 95.0f, 15.0f);
      const bool readOk = prefs.getBytes("comfort", blob, sizeof(blob)) == sizeof(blob);
      const bool temperatureOk = readOk && loadedTemperature.deserialize(blob, ComfortLearner::SERIALIZED_SIZE);
      const bool humidityOk = readOk && loadedHumidity.deserialize(
          blob + ComfortLearner::SERIALIZED_SIZE, ComfortLearner::SERIALIZED_SIZE);
      if (temperatureOk && humidityOk) {
        temperatureComfort = loadedTemperature;
        humidityComfort = loadedHumidity;
        comfortLearnersReady = true;
      }
    }
    prefs.end();
  }
  // A missing or invalid blob leaves both learners at the current zone defaults.
  comfortLearnersReady = true;
}

bool saveComfortLearning() {
  unsigned char blob[COMFORT_BLOB_SIZE];
  if (!temperatureComfort.serialize(blob, ComfortLearner::SERIALIZED_SIZE) ||
      !humidityComfort.serialize(blob + ComfortLearner::SERIALIZED_SIZE,
                                 ComfortLearner::SERIALIZED_SIZE) ||
      !prefs.begin("deskpal", false)) return false;
  const size_t saved = prefs.putBytes("comfort", blob, sizeof(blob));
  prefs.end();
  return saved == sizeof(blob);
}

bool resetComfortLearning() {
  const ClimateParams defaults = getDefaultClimateParams();
  temperatureComfort.init(defaults.tMid - defaults.tHalf, defaults.tMid + defaults.tHalf);
  humidityComfort.init(defaults.hMid - defaults.hHalf, defaults.hMid + defaults.hHalf);
  lastComfortTempUpdated = false;
  lastComfortHumUpdated = false;
  return saveComfortLearning();
}
const char* climateName() {
  if (climateZone == CLIMATE_COASTAL) return "coastal";
  if (climateZone == CLIMATE_DESERT) return "desert";
  if (climateZone == CLIMATE_MOUNTAIN) return "mountain";
  return "temperate";
}

// Linear-regression trend is an estimate in units per hour. Actual sample
// timestamps are used because the sensor cycle is nominally about 2.15 s,
// but loop and I2C timing make the interval vary.
float estimateSlopePerHour(const float* values, int count) {
  if (count < 2) return NAN;
  const int start = historyStartIndex(count);
  const uint32_t baseMs = histTimeMs[start];
  double sumX = 0.0, sumY = 0.0, sumXX = 0.0, sumXY = 0.0;
  for (int i = 0; i < count; ++i) {
    const int idx = historyIndex(start, i);
    const double x = (uint32_t)(histTimeMs[idx] - baseMs) / 3600000.0;
    const double y = values[idx];
    sumX += x; sumY += y; sumXX += x * x; sumXY += x * y;
  }
  const double denominator = count * sumXX - sumX * sumX;
  if (denominator <= 0.0) return NAN;
  return (float)((count * sumXY - sumX * sumY) / denominator);
}

void updateTrendEstimates(unsigned long now) {
  const int available = historySampleCount();
  const int shortCount = min(available, 60);
  tempTrendCPerHr = estimateSlopePerHour(histTemp, shortCount);
  humTrendPerHr = estimateSlopePerHour(histHum, shortCount);
  const bool hasHour = trendDataStartAt != 0 && now - trendDataStartAt >= 3600000UL &&
                       available >= 1000 && (uint32_t)(histTimeMs[(histIdx + HISTORY_CAPACITY - 1) % HISTORY_CAPACITY] - histTimeMs[historyStartIndex(available)]) >= 3600000UL;
  if (!hasHour) {
    pressTrendHpaPerHr = NAN;
    strlcpy(pressureForecast, "Learning", sizeof(pressureForecast));
  } else {
    pressTrendHpaPerHr = estimateSlopePerHour(histPress, available);
    const float projected3h = pressTrendHpaPerHr * 3.0f;
    if (projected3h <= -6.0f) strlcpy(pressureForecast, "Storm risk", sizeof(pressureForecast));
    else if (projected3h <= -1.6f) strlcpy(pressureForecast, "Rain possible", sizeof(pressureForecast));
    else if (projected3h >= 1.6f) strlcpy(pressureForecast, "Fair weather", sizeof(pressureForecast));
    else strlcpy(pressureForecast, "No change", sizeof(pressureForecast));
  }

  // Time to cross the current comfort band is a short-term temperature estimate.
  strlcpy(temperaturePrediction, "Temp estimate: --", sizeof(temperaturePrediction));
  if (sensorsOk && isfinite(tempTrendCPerHr) && fabsf(tempTrendCPerHr) > 0.5f) {
    const ClimateParams zone = getClimateParams();
    const float tMin = zone.tMid - zone.tHalf, tMax = zone.tMid + zone.tHalf;
    float minutes = -1.0f;
    const char* direction = nullptr;
    if (envTemp >= tMin && envTemp <= tMax && tempTrendCPerHr > 0.0f) {
      minutes = (tMax - envTemp) / tempTrendCPerHr * 60.0f;
      direction = "Hot";
    } else if (envTemp >= tMin && envTemp <= tMax && tempTrendCPerHr < 0.0f) {
      minutes = (envTemp - tMin) / -tempTrendCPerHr * 60.0f;
      direction = "Cold";
    }
    if (minutes >= 0.0f && direction) {
      if (minutes < 1.0f) minutes = 1.0f;
      snprintf(temperaturePrediction, sizeof(temperaturePrediction), "%s estimate ~%d min", direction, (int)lroundf(minutes));
    }
  }
}

bool historyValueForWindow(const float* values, uint32_t now, uint32_t windowMs, float& value) {
  const int count = historySampleCount();
  if (count < 2) return false;
  const int start = historyStartIndex(count);
  uint32_t closestAge = UINT32_MAX;
  int closest = -1;
  for (int i = 0; i < count; ++i) {
    const int idx = historyIndex(start, i);
    const uint32_t age = now - histTimeMs[idx];
    if (age >= windowMs && age < closestAge) { closestAge = age; closest = idx; }
  }
  if (closest < 0 || closestAge > windowMs + 15000UL) return false;
  value = values[closest];
  return isfinite(value);
}

void emitInferredEvent(const char* shortLabel, const char* eventMessage, unsigned long now,
                       unsigned long& lastAt, bool& latched) {
  latched = true;
  if (lastAt != 0 && now - lastAt < INFERENCE_COOLDOWN_MS) return;
  lastAt = now;
  strlcpy(inferred, shortLabel, sizeof(inferred));
  emitEvent("Inference", eventMessage);
}

void updateEventInference(unsigned long now) {
  if (!sensorsOk || historySampleCount() < 2) return;
  float temp3m, hum3m, temp10m, hum10m;
  const bool have3m = historyValueForWindow(histTemp, now, 180000UL, temp3m) &&
                      historyValueForWindow(histHum, now, 180000UL, hum3m);
  const bool have10m = historyValueForWindow(histTemp, now, 600000UL, temp10m) &&
                       historyValueForWindow(histHum, now, 600000UL, hum10m);

#if ENABLE_WEATHER
  const WeatherSnapshot outdoor = getWeatherSnapshot();
  const bool outdoorValid = outdoor.ok && isfinite(outdoor.temperature) && outdoor.humidity >= 0 && outdoor.humidity <= 100;
#else
  const bool outdoorValid = false;
#endif
  bool windowLikely = false, windowRelaxed = false;
#if ENABLE_WEATHER
  if (have3m && outdoorValid) {
    const float dt = envTemp - temp3m, dh = envHum - hum3m;
    const bool towardOutdoor = fabsf(outdoor.temperature - envTemp) < fabsf(outdoor.temperature - temp3m) &&
                               fabsf((float)outdoor.humidity - envHum) < fabsf((float)outdoor.humidity - hum3m);
    windowLikely = fabsf(dt) >= 0.8f && fabsf(dh) >= 4.0f && towardOutdoor;
    windowRelaxed = fabsf(dt) >= 0.5f && fabsf(dh) >= 2.0f && towardOutdoor;
  }
#endif
  if (inferredWindowLatched && !windowRelaxed) inferredWindowLatched = false;
  if (!inferredWindowLatched && windowLikely)
    emitInferredEvent("Window/door likely", "Window or door likely opened", now, inferredWindowAt, inferredWindowLatched);

  bool coolingLikely = false, coolingRelaxed = false;
  if (have10m) {
    const float dt = envTemp - temp10m, dh = envHum - hum10m;
    coolingLikely = dt <= -1.0f && dh < 0.0f;
    coolingRelaxed = dt <= -0.6f && dh < 0.0f;
  }
  if (inferredCoolingLatched && !coolingRelaxed) inferredCoolingLatched = false;
  if (!inferredCoolingLatched && coolingLikely)
    emitInferredEvent("Cooling likely on", "AC or cooling likely turned on", now, inferredCoolingAt, inferredCoolingLatched);

  bool occupancyLikely = false, occupancyRelaxed = false;
  const float micGate = micNoiseFloor * 2.5f + 250.0f;
  const bool elevatedMic = micReady && !micMuted && micRms >= micGate;
  if (have10m) {
    const float dt = envTemp - temp10m, dh = envHum - hum10m;
    occupancyLikely = dh >= 3.0f && dt >= 0.3f && elevatedMic;
    occupancyRelaxed = dh >= 1.5f && dt >= 0.15f && elevatedMic;
  }
  if (inferredOccupancyLatched && !occupancyRelaxed) inferredOccupancyLatched = false;
  if (!inferredOccupancyLatched && occupancyLikely)
    emitInferredEvent("Person likely present", "Someone is likely in the room", now, inferredOccupancyAt, inferredOccupancyLatched);
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

bool validAlertLimits(float tmax, float tmin, float hmax) {
  const bool validTempHigh = tmax == 0.0f || (tmax >= -20.0f && tmax <= 60.0f);
  const bool validTempLow = tmin == 0.0f || (tmin >= -20.0f && tmin <= 60.0f);
  const bool validHumidityHigh = hmax == 0.0f || (hmax >= 1.0f && hmax <= 100.0f);
  return isfinite(tmax) && isfinite(tmin) && isfinite(hmax) && validTempHigh &&
         validTempLow && validHumidityHigh &&
         (tmax == 0.0f || tmin == 0.0f || tmin < tmax);
}

bool parseAlertFormValue(const String& text, float& value) {
  if (text.isEmpty()) { value = 0.0f; return true; }
  const char* begin = text.c_str();
  char* end = nullptr;
  value = strtof(begin, &end);
  while (*end && isspace((unsigned char)*end)) ++end;
  return end != begin && *end == '\0' && isfinite(value);
}

void loadConfig() {
  if (!prefs.begin("deskpal", true)) {
    Serial.println("Preferences unavailable; using safe defaults");
    return;
  }
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  if (strlen(DESKPAL_WIFI_SSID) > 0) { wifiSsid = DESKPAL_WIFI_SSID; wifiPass = DESKPAL_WIFI_PASS; }
  apiKey = prefs.getString("apikey", "");
  if (strlen(DESKPAL_WEATHER_API_KEY) > 0) apiKey = DESKPAL_WEATHER_API_KEY;
  city = prefs.getString("city", "");
  countryCode = prefs.getString("country", "");
  alertTmax = prefs.getFloat("alertTmax", ALERT_DEFAULT_TMAX_C);
  alertTmin = prefs.getFloat("alertTmin", ALERT_DEFAULT_TMIN_C);
  alertHmax = prefs.getFloat("alertHmax", ALERT_DEFAULT_HMAX_PCT);
  if (!validAlertLimits(alertTmax, alertTmin, alertHmax)) {
    alertTmax = ALERT_DEFAULT_TMAX_C;
    alertTmin = ALERT_DEFAULT_TMIN_C;
    alertHmax = ALERT_DEFAULT_HMAX_PCT;
    Serial.println("Invalid stored alert limits; using defaults");
  }
  
  tzString = prefs.getString("tz", "IST-5:30");
  
  // FIX: Overwrite old "UTC0" setting in flash memory with IST
  if (tzString == "UTC0" || tzString == "UTC" || tzString.isEmpty()) {
    tzString = "IST-5:30";
    prefs.end();
    if (prefs.begin("deskpal", false)) {
      prefs.putString("tz", tzString);
      prefs.end();
    }
  } else {
    prefs.end();
  }

  climateZone = prefs.getInt("climate", CLIMATE_TEMPERATE);
  tempOffsetC = prefs.getFloat("tempOffset", TEMP_OFFSET);
  if (!isfinite(tempOffsetC) || tempOffsetC < -10.0f || tempOffsetC > 10.0f) tempOffsetC = TEMP_OFFSET;
  
  if (climateZone < CLIMATE_TEMPERATE || climateZone > CLIMATE_MOUNTAIN) {
    climateZone = CLIMATE_TEMPERATE;
    Serial.println("Invalid stored climate; reset to temperate");
  }
  refreshCityUpper();
}

bool setTemperatureOffset(float offsetC) {
  if (!isfinite(offsetC) || offsetC < -10.0f || offsetC > 10.0f) return false;
  if (!prefs.begin("deskpal", false)) return false;
  bool saved = prefs.putFloat("tempOffset", offsetC) > 0;
  prefs.end();
  if (!saved) return false;
  tempOffsetC = offsetC;
  envTemp = NAN;
  histIdx = 0;
  histFilled = false;
  trendDataStartAt = lastTrendSampleAt = 0;
  tempTrendCPerHr = humTrendPerHr = pressTrendHpaPerHr = NAN;
  strlcpy(pressureForecast, "Learning", sizeof(pressureForecast));
  strlcpy(temperaturePrediction, "Temp estimate: --", sizeof(temperaturePrediction));
  char msg[64];
  snprintf(msg, sizeof(msg), "Temperature correction set to %.1f C", tempOffsetC);
  emitEvent("Calibration", msg);
  return true;
}

bool setAlertLimits(float tmax, float tmin, float hmax) {
  if (!validAlertLimits(tmax, tmin, hmax) || !prefs.begin("deskpal", false)) return false;
  const bool saved = prefs.putFloat("alertTmax", tmax) > 0 &&
                     prefs.putFloat("alertTmin", tmin) > 0 &&
                     prefs.putFloat("alertHmax", hmax) > 0;
  prefs.end();
  if (!saved) return false;
  alertTmax = tmax;
  alertTmin = tmin;
  alertHmax = hmax;
  alertedTmax = alertedTmin = alertedHmax = false;
  lastAlertTmaxAt = lastAlertTminAt = lastAlertHmaxAt = 0;
  return true;
}

static bool putStr(const char* key, const String& value) {
  return prefs.putString(key, value) > 0 || value.length() == 0;
}

bool saveConfig(const String& s, const String& p, const String& ak, const String& cty, const String& ctry, const String& tz, int cl,
                float tmax, float tmin, float hmax) {
  if (!prefs.begin("deskpal", false)) return false;
  const String savedTimezone = tz.isEmpty() ? "IST-5:30" : tz;
  bool ok = putStr("ssid", s) && putStr("pass", p) &&
            putStr("apikey", ak) && putStr("city", cty) &&
            putStr("country", ctry) && putStr("tz", savedTimezone) &&
            prefs.putInt("climate", cl) > 0 &&
            prefs.putFloat("alertTmax", tmax) > 0 &&
            prefs.putFloat("alertTmin", tmin) > 0 &&
            prefs.putFloat("alertHmax", hmax) > 0;
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
  String sZ = htmlEscape(prefs.getString("tz", "IST-5:30")); 
  
  int cl = prefs.getInt("climate", 0);
  float tmax = alertTmax;
  float tmin = alertTmin;
  float hmax = alertHmax;
  prefs.end();
  String s0 = cl == 0 ? " selected" : "", s1 = cl == 1 ? " selected" : "";
  String s2 = cl == 2 ? " selected" : "", s3 = cl == 3 ? " selected" : "";
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DeskPal Config</title><style>
body{font-family:sans-serif;max-width:420px;margin:30px auto;padding:24px;background:#0c1929;color:#e8f4fc;}
h1{color:#5ba3f5;}input,select{width:100%;padding:10px;margin:6px 0;border:1px solid #2d4a6f;border-radius:6px;box-sizing:border-box;background:#1a2d47;color:#e8f4fc;}
button{width:100%;padding:12px;background:#3498db;color:#fff;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin-top:16px;}
label{display:block;margin-top:12px;color:#8ab4e8;font-size:14px;}small{display:block;color:#9bb5d3;margin:0 0 6px;}</style></head><body>
<h1>DeskPal Setup</h1><form action="/save" method="POST">
<p>Dashboard pairing code: <code>)rawliteral" + String(pairCode) + R"rawliteral(</code></p>
<p>Dashboard access token (keep private): <code>)rawliteral" + String(telemetryToken) + R"rawliteral(</code></p>
<label>WiFi SSID</label><input name="ssid" value=")rawliteral" + sS + R"rawliteral(">
<label>WiFi Password</label><input name="pass" type="password">
<label>OpenWeather API Key</label><input name="apikey" type="password" autocomplete="new-password">
<label>City</label><input name="city" value=")rawliteral" + sC + R"rawliteral(">
<label>Country Code (2 letters, optional)</label><input name="country" value=")rawliteral" + sT + R"rawliteral(">
<label>Timezone (POSIX, e.g., IST-5:30)</label><input name="tz" value=")rawliteral" + sZ + R"rawliteral(">
<small>Use IST-5:30 for India.</small>
<label>Climate zone</label><select name="climate">
<option value="0")rawliteral" + s0 + R"rawliteral(>Temperate</option>
<option value="1")rawliteral" + s1 + R"rawliteral(>Coastal / Mumbai</option>
<option value="2")rawliteral" + s2 + R"rawliteral(>Desert / Rajasthan</option>
<option value="3")rawliteral" + s3 + R"rawliteral(>Mountain / Kashmir</option>
</select>
<p>Alert limits (enter 0 to turn a rule off)</p>
<label>Temperature high (C)</label><input name="alertTmax" type="number" min="-20" max="60" step="0.1" value=")rawliteral" + String(tmax, 1) + R"rawliteral(">
<label>Temperature low (C)</label><input name="alertTmin" type="number" min="-20" max="60" step="0.1" value=")rawliteral" + String(tmin, 1) + R"rawliteral(">
<label>Humidity high (%)</label><input name="alertHmax" type="number" min="0" max="100" step="1" value=")rawliteral" + String(hmax, 1) + R"rawliteral(">
<button type="submit">Save &amp; Reboot</button></form>
<form action="/reset" method="POST" onsubmit="return confirm('Erase all saved settings?')">
<button type="submit">Erase saved settings</button></form></body></html>)rawliteral";
  configServer.send(200, "text/html", html);
}

void handleConfigSave() {
  String ssid = configServer.arg("ssid"), pass = configServer.arg("pass");
  String key = configServer.arg("apikey"), cty = configServer.arg("city");
  String country = configServer.arg("country"), tz = configServer.arg("tz");
  float tmax = alertTmax, tmin = alertTmin, hmax = alertHmax;
  if (ssid.isEmpty() || ssid.length() > 32 || pass.length() > 63 || key.length() > 128 ||
      cty.length() > 64 || tz.length() > 64 || !(country.isEmpty() || country.length() == 2)) {
    configServer.send(400, "text/plain", "Invalid configuration field length or country code"); return;
  }
  for (size_t i = 0; i < country.length(); ++i) {
    if (!isalpha((unsigned char)country[i])) { configServer.send(400, "text/plain", "Country code must contain two letters"); return; }
    country[i] = toupper((unsigned char)country[i]);
  }
  int cl = configServer.hasArg("climate") ? configServer.arg("climate").toInt() : CLIMATE_TEMPERATE;
  if (cl < CLIMATE_TEMPERATE || cl > CLIMATE_MOUNTAIN) { configServer.send(400, "text/plain", "Invalid climate zone"); return; }
  if ((configServer.hasArg("alertTmax") && !parseAlertFormValue(configServer.arg("alertTmax"), tmax)) ||
      (configServer.hasArg("alertTmin") && !parseAlertFormValue(configServer.arg("alertTmin"), tmin)) ||
      (configServer.hasArg("alertHmax") && !parseAlertFormValue(configServer.arg("alertHmax"), hmax)) ||
      !validAlertLimits(tmax, tmin, hmax)) {
    configServer.send(400, "text/plain", "Alert limits must be 0 (off), temperature -20 to 60 C, humidity 1 to 100%, and low temperature must be below high temperature");
    return;
  }
  if (pass.isEmpty() || key.isEmpty()) {
    prefs.begin("deskpal", true);
    if (pass.isEmpty()) pass = prefs.getString("pass", "");
    if (key.isEmpty()) key = prefs.getString("apikey", "");
    prefs.end();
  }
  if (pass.length() > 0 && pass.length() < 8) { configServer.send(400, "text/plain", "Wi-Fi password must be empty or at least 8 characters"); return; }
  
  if (!saveConfig(ssid, pass, key, cty, country, tz.isEmpty() ? "IST-5:30" : tz, cl, tmax, tmin, hmax)) {
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
  const char* headerNames[] = { "X-DeskPal-Token" };
  configServer.collectHeaders(headerNames, 1);
  configServer.on("/api/telemetry", HTTP_GET, sendTelemetryApi);
  configServer.on("/api/telemetry", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/events", HTTP_GET, sendEventsApi);
  configServer.on("/api/events", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/wifi", HTTP_POST, handleWifiSaveApi);
  configServer.on("/api/wifi", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/timer", HTTP_POST, handleTimerApi);
  configServer.on("/api/timer", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/calibration", HTTP_POST, handleCalibrationApi);
  configServer.on("/api/calibration", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/alerts", HTTP_POST, handleAlertsApi);
  configServer.on("/api/alerts", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/comfort", HTTP_POST, handleComfortApi);
  configServer.on("/api/comfort", HTTP_OPTIONS, handleCorsOptions);
  configServer.onNotFound([]() {
    configServer.sendHeader("Access-Control-Allow-Origin", DESKPAL_CORS_ORIGIN);
    configServer.send(404, "application/json", "{\"error\":\"not found\"}");
  });
  configServer.begin();
  telemetryServerStarted = true;
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

bool recordComfortVote(bool humidity, ComfortLearner::Label label) {
  if (!comfortLearnersReady || !sensorsOk || !isfinite(envTemp) || !isfinite(envHum) ||
      label < ComfortLearner::HOT || label > ComfortLearner::PERFECT) return false;
  const unsigned long now = millis();
  if (hasLastComfortMutation && now - lastComfortMutationAt < 10000UL) return false;

  const bool updateTemperature = !humidity || label == ComfortLearner::PERFECT;
  const bool updateHumidity = humidity || label == ComfortLearner::PERFECT;
  const bool temperatureVoted = updateTemperature && temperatureComfort.vote(envTemp, label);
  const bool humidityVoted = updateHumidity && humidityComfort.vote(envHum, label);
  if ((updateTemperature && !temperatureVoted) || (updateHumidity && !humidityVoted)) {
    if (temperatureVoted) temperatureComfort.undoLast();
    if (humidityVoted) humidityComfort.undoLast();
    return false;
  }
  if (!saveComfortLearning()) {
    if (temperatureVoted) temperatureComfort.undoLast();
    if (humidityVoted) humidityComfort.undoLast();
    return false;
  }

  hasLastComfortMutation = true;
  lastComfortMutationAt = now;
  lastComfortTempUpdated = updateTemperature;
  lastComfortHumUpdated = updateHumidity;
  manualMood = MOOD_HAPPY;
  manualMoodUntil = now + 2000UL;
  buzzTap();
  char message[96];
  snprintf(message, sizeof(message), "Learned: %.1f-%.1f C (%lu votes)",
           temperatureComfort.lo(), temperatureComfort.hi(),
           (unsigned long)(temperatureComfort.votes() + humidityComfort.votes()));
  emitEvent("Comfort", message);
  return true;
}

enum ComfortCommandResult {
  COMFORT_COMMAND_INVALID,
  COMFORT_COMMAND_REJECTED,
  COMFORT_COMMAND_OK
};

bool undoComfortLearning() {
  const unsigned long now = millis();
  if (hasLastComfortMutation && now - lastComfortMutationAt < 10000UL) return false;
  if (!lastComfortTempUpdated && !lastComfortHumUpdated) return false;

  const ComfortLearner oldTemperature = temperatureComfort;
  const ComfortLearner oldHumidity = humidityComfort;
  const bool temperatureUndone =
      !lastComfortTempUpdated || temperatureComfort.undoLast();
  const bool humidityUndone =
      !lastComfortHumUpdated || humidityComfort.undoLast();
  if (!temperatureUndone || !humidityUndone || !saveComfortLearning()) {
    temperatureComfort = oldTemperature;
    humidityComfort = oldHumidity;
    return false;
  }

  hasLastComfortMutation = true;
  lastComfortMutationAt = now;
  lastComfortTempUpdated = false;
  lastComfortHumUpdated = false;
  emitEvent("Comfort", "Previous comfort vote undone");
  return true;
}

int processComfortCommand(JsonObject request) {
  if (!request["cmd"].is<const char*>()) return COMFORT_COMMAND_INVALID;
  const char* command = request["cmd"].as<const char*>();

  if (strcmp(command, "vote") == 0) {
    if (request.size() != 3 || !request["label"].is<const char*>() ||
        !request["kind"].is<const char*>()) return COMFORT_COMMAND_INVALID;

    const char* labelText = request["label"].as<const char*>();
    const char* kindText = request["kind"].as<const char*>();
    if (strlen(labelText) > 7 || strlen(kindText) > 4)
      return COMFORT_COMMAND_INVALID;

    ComfortLearner::Label label;
    if (strcmp(labelText, "hot") == 0) label = ComfortLearner::HOT;
    else if (strcmp(labelText, "cold") == 0) label = ComfortLearner::COLD;
    else if (strcmp(labelText, "perfect") == 0) label = ComfortLearner::PERFECT;
    else return COMFORT_COMMAND_INVALID;

    bool humidity;
    if (strcmp(kindText, "temp") == 0) humidity = false;
    else if (strcmp(kindText, "hum") == 0) humidity = true;
    else return COMFORT_COMMAND_INVALID;

    return recordComfortVote(humidity, label)
        ? COMFORT_COMMAND_OK : COMFORT_COMMAND_REJECTED;
  }

  if (strcmp(command, "undo") == 0) {
    if (request.size() != 1) return COMFORT_COMMAND_INVALID;
    return undoComfortLearning() ? COMFORT_COMMAND_OK : COMFORT_COMMAND_REJECTED;
  }

  if (strcmp(command, "reset_comfort") == 0) {
    if (request.size() != 1) return COMFORT_COMMAND_INVALID;
    const unsigned long now = millis();
    if (hasLastComfortMutation && now - lastComfortMutationAt < 10000UL)
      return COMFORT_COMMAND_REJECTED;
    if (!resetComfortLearning()) return COMFORT_COMMAND_REJECTED;

    hasLastComfortMutation = true;
    lastComfortMutationAt = now;
    lastComfortTempUpdated = false;
    lastComfortHumUpdated = false;
    emitEvent("Comfort", "Comfort learning reset");
    return COMFORT_COMMAND_OK;
  }

  return COMFORT_COMMAND_INVALID;
}

void emitThresholdAlert(const char* message, unsigned long now, unsigned long& lastAt, bool& alerted) {
  if (alerted && now - lastAt < ALERT_REPEAT_MS) return;
  lastAt = now;
  alerted = true;
  emitEvent("Alert", message);
  buzzAlert();
}

void checkAlertThresholds() {
  if (!sensorsOk || buzzQueueIdx >= 0) return;
  const unsigned long now = millis();
  char message[96];
  if (alertTmax != 0.0f && envTemp >= alertTmax &&
      (!alertedTmax || now - lastAlertTmaxAt >= ALERT_REPEAT_MS)) {
    snprintf(message, sizeof(message), "Temperature above limit: %.1f C (limit %.1f C)", envTemp, alertTmax);
    emitThresholdAlert(message, now, lastAlertTmaxAt, alertedTmax);
    return;
  }
  if (alertTmin != 0.0f && envTemp <= alertTmin &&
      (!alertedTmin || now - lastAlertTminAt >= ALERT_REPEAT_MS)) {
    snprintf(message, sizeof(message), "Temperature below limit: %.1f C (limit %.1f C)", envTemp, alertTmin);
    emitThresholdAlert(message, now, lastAlertTminAt, alertedTmin);
    return;
  }
  if (alertHmax != 0.0f && envHum >= alertHmax &&
      (!alertedHmax || now - lastAlertHmaxAt >= ALERT_REPEAT_MS)) {
    snprintf(message, sizeof(message), "Humidity above limit: %d%% (limit %.0f%%)", (int)envHum, alertHmax);
    emitThresholdAlert(message, now, lastAlertHmaxAt, alertedHmax);
  }
}

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
  anomalyMsg[0] = '\0';

  // Compare each new reading with the previous 20 readings, then replace the
  // oldest entry so a sustained change gradually becomes the new baseline.
  if (anomalyWindowCount == ANOMALY_WINDOW_SIZE) {
    float sums[3] = { 0.0f, 0.0f, 0.0f };
    for (uint8_t i = 0; i < ANOMALY_WINDOW_SIZE; ++i) {
      sums[0] += anomalyTempWindow[i];
      sums[1] += anomalyHumWindow[i];
      sums[2] += anomalyPressWindow[i];
    }
    const float means[3] = {
      sums[0] / ANOMALY_WINDOW_SIZE,
      sums[1] / ANOMALY_WINDOW_SIZE,
      sums[2] / ANOMALY_WINDOW_SIZE
    };
    float squaredDiffs[3] = { 0.0f, 0.0f, 0.0f };
    for (uint8_t i = 0; i < ANOMALY_WINDOW_SIZE; ++i) {
      const float dt = anomalyTempWindow[i] - means[0];
      const float dh = anomalyHumWindow[i] - means[1];
      const float dp = anomalyPressWindow[i] - means[2];
      squaredDiffs[0] += dt * dt;
      squaredDiffs[1] += dh * dh;
      squaredDiffs[2] += dp * dp;
    }
    const float sdTemp = fmaxf(sqrtf(squaredDiffs[0] / ANOMALY_WINDOW_SIZE), ANOMALY_TEMP_SD_FLOOR);
    const float sdHum = fmaxf(sqrtf(squaredDiffs[1] / ANOMALY_WINDOW_SIZE), ANOMALY_HUM_SD_FLOOR);
    const float sdPress = fmaxf(sqrtf(squaredDiffs[2] / ANOMALY_WINDOW_SIZE), ANOMALY_PRESS_SD_FLOOR);
    const float zTemp = fabsf(t - means[0]) / sdTemp;
    const float zHum = fabsf(h - means[1]) / sdHum;
    const float zPress = fabsf(p - means[2]) / sdPress;

    if (zTemp > ANOMALY_TEMP_Z_LIMIT) strlcpy(anomalyMsg, "Room temperature changed quickly", sizeof(anomalyMsg));
    else if (zHum > ANOMALY_HUM_Z_LIMIT) strlcpy(anomalyMsg, "Room humidity changed quickly", sizeof(anomalyMsg));
    else if (zPress > ANOMALY_PRESS_Z_LIMIT && millis() > liftSuppressUntil)
      strlcpy(anomalyMsg, "Room pressure changed quickly", sizeof(anomalyMsg));

    const unsigned long now = millis();
    if (anomalyMsg[0] != '\0' && (lastAnomalyNoticeAt == 0 || now - lastAnomalyNoticeAt >= 60000UL)) {
      anomalyUntil = now + 5000UL;
      lastAnomalyNoticeAt = now;
      emitDecision(anomalyMsg);
    } else {
      anomalyMsg[0] = '\0';
    }
  }

  anomalyTempWindow[anomalyWindowIndex] = t;
  anomalyHumWindow[anomalyWindowIndex] = h;
  anomalyPressWindow[anomalyWindowIndex] = p;
  anomalyWindowIndex = (anomalyWindowIndex + 1) % ANOMALY_WINDOW_SIZE;
  if (anomalyWindowCount < ANOMALY_WINDOW_SIZE) ++anomalyWindowCount;
}

bool isRelativelyCold() {
  if (isnan(envTemp)) return false;
  ClimateParams z = getClimateParams();
  return envTemp < (z.tMid - z.tHalf);
}
void generateInsights() {
  if (!sensorsOk || isnan(envTemp) || isnan(envHum)) {
    strlcpy(insightTitle, "ROOM CHECK", sizeof(insightTitle));
    strlcpy(insightAction, "Sensor data unavailable", sizeof(insightAction));
    return;
  }
  if (strcmp(moldRisk, "HIGH") == 0) {
    strlcpy(insightTitle, "HIGH ROOM RISK", sizeof(insightTitle));
    strlcpy(insightAction, envTemp - dewPointC <= 2.0f ? "Condensation likely" : "Mold growth likely",
            sizeof(insightAction));
    return;
  }
  if (strcmp(moldRisk, "MEDIUM") == 0) {
    strlcpy(insightTitle, "MOLD RISK ESTIMATE", sizeof(insightTitle));
    strlcpy(insightAction, "Mold growth likely", sizeof(insightAction));
    return;
  }
  ClimateParams z = getClimateParams();
  float tMin = z.tMid - z.tHalf, tMax = z.tMid + z.tHalf;
  if (envTemp > tMax) {
    strlcpy(insightTitle, "ROOM IS WARM", sizeof(insightTitle));
    strlcpy(insightAction, "Ventilate or cool", sizeof(insightAction));
  } else if (envTemp < tMin) {
    strlcpy(insightTitle, "ROOM IS COOL", sizeof(insightTitle));
    strlcpy(insightAction, "Add warmth if needed", sizeof(insightAction));
  } else if (envHum > z.hMid + z.hHalf) {
    strlcpy(insightTitle, "HUMIDITY IS HIGH", sizeof(insightTitle));
    strlcpy(insightAction, "Ventilate the room", sizeof(insightAction));
  } else if (envHum < z.hMid - z.hHalf) {
    strlcpy(insightTitle, "AIR IS DRY", sizeof(insightTitle));
    strlcpy(insightAction, "Add some humidity", sizeof(insightAction));
  } else {
    strlcpy(insightTitle, "ROOM COMFORTABLE", sizeof(insightTitle));
    strlcpy(insightAction, "Temp and humidity OK", sizeof(insightAction));
  }
}
void computeRoomAnalysis() {
  if (!sensorsOk || !isfinite(envTemp) || !isfinite(envHum)) {
    dewPointC = heatIndexC = comfortScore = NAN;
    strlcpy(moldRisk, "LOW", sizeof(moldRisk));
    humidityAbove70Tracking = humidityAbove80Tracking = false;
    return;
  }

  // Magnus dew point is an estimate; clamp RH away from zero for the logarithm.
  const float rh = cf(envHum, 1.0f, 100.0f);
  const float gamma = logf(rh / 100.0f) + DEW_POINT_A * envTemp / (DEW_POINT_B + envTemp);
  dewPointC = DEW_POINT_B * gamma / (DEW_POINT_A - gamma);

  // Rothfusz regression heat index is an estimate and is used in its hot/humid range.
  if (envTemp >= HEAT_INDEX_MIN_TEMP_C && rh >= HEAT_INDEX_MIN_HUMIDITY) {
    const float f = envTemp * 9.0f / 5.0f + 32.0f;
    const float r = rh;
    float hiF = -42.379f + 2.04901523f * f + 10.14333127f * r - 0.22475541f * f * r
                - 0.00683783f * f * f - 0.05481717f * r * r
                + 0.00122874f * f * f * r + 0.00085282f * f * r * r
                - 0.00000199f * f * f * r * r;
    if (r < 13.0f && f >= 80.0f && f <= 112.0f)
      hiF -= ((13.0f - r) / 4.0f) * sqrtf(fmaxf(0.0f, (17.0f - fabsf(f - 95.0f)) / 17.0f));
    else if (r > 85.0f && f >= 80.0f && f <= 87.0f)
      hiF += ((r - 85.0f) / 10.0f) * ((87.0f - f) / 5.0f);
    heatIndexC = (hiF - 32.0f) * 5.0f / 9.0f;
  } else {
    heatIndexC = envTemp;
  }

  const ClimateParams zone = getClimateParams();
  const float tMin = zone.tMid - zone.tHalf, tMax = zone.tMid + zone.tHalf;
  const float hMin = zone.hMid - zone.hHalf, hMax = zone.hMid + zone.hHalf;
  const float tempDistance = envTemp < tMin ? tMin - envTemp : (envTemp > tMax ? envTemp - tMax : 0.0f);
  const float humDistance = rh < hMin ? hMin - rh : (rh > hMax ? rh - hMax : 0.0f);
  const float tempPart = cf(100.0f - 10.0f * tempDistance, 0.0f, 100.0f);
  const float humidityPart = cf(100.0f - 1.5f * humDistance, 0.0f, 100.0f);
  const float dewPenalty = dewPointC >= COMFORT_OPPRESSIVE_DEW_C
      ? cf(25.0f + (dewPointC - COMFORT_OPPRESSIVE_DEW_C) * 10.0f, 25.0f, 100.0f)
      : 0.0f;
  comfortScore = roundf(0.55f * tempPart + 0.25f * humidityPart + 0.20f * (100.0f - dewPenalty));

  // Risk levels are estimates based on sustained RH timers and near-saturation dew point.
  const unsigned long now = millis();
  if (rh > 70.0f) {
    if (!humidityAbove70Tracking) { humidityAbove70Since = now; humidityAbove70Tracking = true; }
  } else humidityAbove70Tracking = false;
  if (rh > 80.0f) {
    if (!humidityAbove80Tracking) { humidityAbove80Since = now; humidityAbove80Tracking = true; }
  } else humidityAbove80Tracking = false;

  const bool sustained70 = humidityAbove70Tracking && now - humidityAbove70Since >= HUMIDITY_RISK_SUSTAINED_MS;
  const bool sustained80 = humidityAbove80Tracking && now - humidityAbove80Since >= HUMIDITY_RISK_SUSTAINED_MS;
  if (sustained80 || envTemp - dewPointC <= 2.0f) strlcpy(moldRisk, "HIGH", sizeof(moldRisk));
  else if (sustained70) strlcpy(moldRisk, "MEDIUM", sizeof(moldRisk));
  else strlcpy(moldRisk, "LOW", sizeof(moldRisk));
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
    if (millis() - lastGoodSensorRead > SENSOR_GRACE_MS) {
      sensorsOk = false;
      computeRoomAnalysis();
      if (!sensorUnavailableEventSent) { sensorUnavailableEventSent = true; emitEvent("Sensor", "Room sensor data unavailable"); }
    }
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
    if (millis() - lastGoodSensorRead > SENSOR_GRACE_MS) {
      sensorsOk = false;
      computeRoomAnalysis();
      if (!sensorUnavailableEventSent) { sensorUnavailableEventSent = true; emitEvent("Sensor", "Room sensor data unavailable"); }
    }
    return;
  }
  envTempRaw = tSum / validReadings;
  float t = envTempRaw + tempOffsetC;
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
  computeRoomAnalysis();
  runInference();
  anomalyActive = (millis() < anomalyUntil);
  const unsigned long sampleNow = millis();
  if (lastTrendSampleAt != 0 && sampleNow - lastTrendSampleAt > 10000UL) {
    histIdx = 0;
    histFilled = false;
  }
  if (lastTrendSampleAt == 0 || sampleNow - lastTrendSampleAt > 10000UL) trendDataStartAt = sampleNow;
  lastTrendSampleAt = sampleNow;
  histTemp[histIdx] = envTemp;
  histHum[histIdx] = envHum;
  histPress[histIdx] = envPress;
  histTimeMs[histIdx] = (uint32_t)sampleNow;
  histIdx = (histIdx + 1) % HISTORY_CAPACITY;
  if (histIdx == 0) histFilled = true;
  updateTrendEstimates(sampleNow);
  updateEventInference(sampleNow);
  if (sensorUnavailableEventSent) {
    sensorUnavailableEventSent = false;
    emitEvent("Sensor", "Room sensor data recovered");
  }
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
      emitDecision("Picked up - scared");
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
  static int lastLoggedMood = -1;
  if (m != lastLoggedMood) {
    char moodMessage[48];
    snprintf(moodMessage, sizeof(moodMessage), "Mood changed to %s", MOOD_NAMES[m]);
    emitEvent("Mood", moodMessage);
    lastLoggedMood = m;
  }
  switch (m) {
    case MOOD_SCARED: pupilScale = 0.5f; break;
    case MOOD_LOVE: pupilScale = 1.25f; break;
    case MOOD_SURPRISED: pupilScale = 0.8f; break;
    default: pupilScale = 1.0f;
  }
}

// MiniAI uses a weighted decision matrix: each normalized signal is multiplied
// by its named weight, then the highest-scoring candidate is selected.
static const float MINIAI_HOT_C = 27.0f;
static const float MINIAI_COLD_C = 22.0f;
static const float MINIAI_TEMP_SCALE_C = 3.0f;
static const float MINIAI_HUM_HIGH = 75.0f;
static const float MINIAI_HUM_LOW = 30.0f;
static const float MINIAI_HUM_SCALE = 15.0f;
static const float MINIAI_NOISE_SCALE = 3.0f;
static const float MINIAI_NOISE_GATE = 2.0f;
static const float MINIAI_QUIET_CEILING = 200.0f;
static const float MINIAI_PRESSURE_SCALE = 2.0f;
static const float MINIAI_MIN_SCORE = 0.15f;
static const float MINIAI_W_TEMP = 1.0f;
static const float MINIAI_W_HUM = 1.0f;
static const float MINIAI_W_NOISE = 1.0f;
static const float MINIAI_W_PRESSURE = 0.9f;
static const float MINIAI_W_TIMER = 1.0f;
static const float MINIAI_W_ANOMALY = 1.0f;
static const unsigned long MINIAI_MIN_INTERVAL_MS = 30000UL;

void triggerMiniAI() {
  static unsigned long lastRunAt = 0;
  static bool hasRun = false;
  const unsigned long now = millis();
  if (hasRun && now - lastRunAt < MINIAI_MIN_INTERVAL_MS) return;
  const unsigned long startTime = micros();
  hasRun = true;
  lastRunAt = now;
  isAiResponding = true; aiResponseStart = now;
  if (!sensorsOk && !timerRunning && !anomalyActive) {
    strlcpy(aiResponse, "Room sensor unavailable; check connections.", sizeof(aiResponse));
    Serial.print("MiniAI us="); Serial.println(micros() - startTime);
    return;
  }
  float sHot = sensorsOk && !isnan(envTemp) ? cf((envTemp - MINIAI_HOT_C) / MINIAI_TEMP_SCALE_C, 0, 1) : 0.0f;
  float sCold = sensorsOk && !isnan(envTemp) ? cf((MINIAI_COLD_C - envTemp) / MINIAI_TEMP_SCALE_C, 0, 1) : 0.0f;
  float sHum = sensorsOk && !isnan(envHum) ? cf((envHum - MINIAI_HUM_HIGH) / MINIAI_HUM_SCALE, 0, 1) : 0.0f;
  float sDry = sensorsOk && !isnan(envHum) ? cf((MINIAI_HUM_LOW - envHum) / MINIAI_HUM_SCALE, 0, 1) : 0.0f;
  float sNoise = cf((micRms - micNoiseFloor * MINIAI_NOISE_GATE) / (micNoiseFloor * MINIAI_NOISE_SCALE), 0, 1);
  float sQuiet = (micReady ? 1.0f : 0.0f) * cf((MINIAI_QUIET_CEILING - micRms) / MINIAI_QUIET_CEILING, 0, 1);
  float sRain = (!isnan(envPress) && !isnan(pressHourAgo)) ? cf((pressHourAgo - envPress) / MINIAI_PRESSURE_SCALE, 0, 1) : 0.0f;
  float sTimer = timerRunning ? 1.0f : 0.0f;
  float sAnom = anomalyActive ? 1.0f : 0.0f;
  char cands[8][72]; float scores[8];
  snprintf(cands[0], sizeof(cands[0]), "Timer running: %lu min left. Focus!", timerRemainingSec / 60); scores[0] = sTimer * MINIAI_W_TIMER;
  snprintf(cands[1], sizeof(cands[1]), "Env shifting! %s", anomalyMsg); scores[1] = sAnom * MINIAI_W_ANOMALY;
  if (sensorsOk && !isnan(envTemp)) {
    snprintf(cands[2], sizeof(cands[2]), "It's %dC - warm. Hydrate, human!", (int)envTemp);
    snprintf(cands[3], sizeof(cands[3]), "It's %dC - chilly. Grab a jacket!", (int)envTemp);
  } else {
    strlcpy(cands[2], "Room sensor unavailable.", sizeof(cands[2]));
    strlcpy(cands[3], "Room sensor unavailable.", sizeof(cands[3]));
  }
  if (sensorsOk && !isnan(envHum)) snprintf(cands[4], sizeof(cands[4]), "Humidity %d%% - fan time!", (int)envHum);
  else strlcpy(cands[4], "Room sensor unavailable.", sizeof(cands[4]));
  scores[2] = sHot * MINIAI_W_TEMP; scores[3] = sCold * MINIAI_W_TEMP; scores[4] = sHum * MINIAI_W_HUM;
  snprintf(cands[5], sizeof(cands[5]), "Air is dry - sip some water!"); scores[5] = sDry * MINIAI_W_HUM;
  snprintf(cands[6], sizeof(cands[6]), "Room noisy - find a quiet spot!"); scores[6] = sNoise * MINIAI_W_NOISE;
  if (sRain > 0.3f) snprintf(cands[7], sizeof(cands[7]), "Pressure falling - rain likely soon.");
  else snprintf(cands[7], sizeof(cands[7]), "Room quiet & stable. Perfect for deep work!");
  scores[7] = max(sQuiet, sRain * MINIAI_W_PRESSURE);
  int best = 0;
  for (int i = 1; i < 8; i++) if (scores[i] > scores[best]) best = i;
  if (scores[best] < MINIAI_MIN_SCORE) best = 7;
  strlcpy(aiResponse, cands[best], sizeof(aiResponse));
  char aiEvent[112];
  snprintf(aiEvent, sizeof(aiEvent), "MiniAI: %s", aiResponse);
  emitDecision(aiEvent);
  Serial.print("MiniAI us="); Serial.println(micros() - startTime);
}

void processVoiceCommand() {
  isAiResponding = true; aiResponseStart = millis(); currentPage = PAGE_KWS;
  if (commandSyllables == 0) strlcpy(aiResponse, "No speech heard. Timer unchanged.", sizeof(aiResponse));
  else strlcpy(aiResponse, "Speech not recognized. Timer unchanged.", sizeof(aiResponse));
}

void keywordSpotted() {
  kwsActiveUntil = millis() + 2500; excitedUntil = millis() + 2500; lastInteraction = millis(); buzzKws();
  emitDecision("Wake word detected");
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
    emitDecision("Voice command: start timer");
  } else if (label == KWS_LABEL_STOP_TIMER) {
    timerRunning = false; timerLock = false;
    strlcpy(aiResponse, "Focus timer stopped", sizeof(aiResponse)); buzzTap();
    emitDecision("Voice command: stop timer");
  } else return;
  isAiResponding = true; aiResponseStart = now; currentPage = PAGE_KWS;
}
#endif

void petAction(unsigned long now, bool strongRub) {
  lastInteraction = now;
  if (neglectSleepy) { excitedUntil = now + 3000; buzzHappy(); emitEvent("Touch", "DeskPal was petted awake"); return; }
  if (now < annoyedUntil) { annoyedUntil = now + 4000; buzzGrumble(); emitEvent("Touch", "Petting while annoyed"); return; }
  if (now - petWindowStart > PET_WINDOW_MS) { petWindowStart = now; petCount = 0; }
  petCount++;
  if (petCount >= PET_LIMIT) { annoyedUntil = now + 5000; petCount = 0; buzzGrumble(); emitDecision("Over-petted - angry"); }
  else { pettedUntil = now + (strongRub ? 5000UL : 3000UL); buzzHappy(); emitEvent("Touch", strongRub ? "Strong pet detected" : "DeskPal was petted"); }
}
void longPetAction(unsigned long now) {
  lastInteraction = now; annoyedUntil = 0; petCount = 0; pettedUntil = now + 6000; buzzHappy();
  emitDecision("Long cuddle - calmed");
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
  readMicRms();
#if USE_TFLITE
  int label = classifyKeywordEdge();
  handleKeywordLabel(label);
#endif
}

float readBatteryPct() {
#if ENABLE_BATTERY
  int raw = analogRead(BATT_PIN);
  float v = (raw / 4095.0f) * 3.3f * 2.0f;
  return cf((v - 3.0f) / 1.2f * 100.0f, 0.0f, 100.0f);
#else
  return NAN;
#endif
}
const char* pageName(int page) {
  static const char* names[] = {"Eyes", "Clock", "Weather", "Forecast", "Room", "Insights", "Microphone", "Timer"};
  return (page >= PAGE_EYES && page <= PAGE_TIMER) ? names[page] : "Unknown";
}
void emitDashboardTelemetry() {
  static unsigned long lastEmit = 0;
  if (millis() - lastEmit < 1000) return;
  lastEmit = millis(); batteryPct = readBatteryPct();
  static unsigned long lastHeapReportAt = 0;
  if (lastHeapReportAt == 0 || lastEmit - lastHeapReportAt >= 60000UL) {
    lastHeapReportAt = lastEmit;
    Serial.printf("Heap free=%u min=%u\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
  }
  static int lastLoggedPage = -1;
  if (currentPage != lastLoggedPage) {
    char pageMessage[40];
    snprintf(pageMessage, sizeof(pageMessage), "Page changed to %s", pageName(currentPage));
    emitEvent("Navigation", pageMessage);
    lastLoggedPage = currentPage;
  }
  static bool lastLoggedAnomaly = false;
  if (anomalyActive != lastLoggedAnomaly) {
    emitEvent("Anomaly", anomalyActive ? "Sensor anomaly active" : "Sensor anomaly cleared");
    lastLoggedAnomaly = anomalyActive;
  }
  Serial.print("{\"type\":\"telemetry\",\"schema\":1,\"uptimeMs\":"); Serial.print(millis());
  Serial.print(",\"sensorsOk\":"); Serial.print(sensorsOk ? "true" : "false");
  Serial.print(",\"sensorAgeMs\":"); if (lastGoodSensorRead) Serial.print(millis() - lastGoodSensorRead); else Serial.print("null");
  Serial.print(",\"micReady\":"); Serial.print(micReady ? "true" : "false");
  Serial.print(",\"modelReady\":"); Serial.print(KWS_MODEL_INTEGRATED ? "true" : "false");
  Serial.print(",\"t\":"); if (!sensorsOk || isnan(envTemp)) Serial.print("null"); else Serial.print(envTemp, 1);
  Serial.print(",\"tRaw\":"); if (!sensorsOk || isnan(envTempRaw)) Serial.print("null"); else Serial.print(envTempRaw, 1);
  Serial.print(",\"tempOffsetC\":"); Serial.print(tempOffsetC, 1);
  Serial.print(",\"h\":"); if (!sensorsOk || isnan(envHum)) Serial.print("null"); else Serial.print((int)envHum);
  Serial.print(",\"p\":"); if (!sensorsOk || isnan(envPress)) Serial.print("null"); else Serial.print((int)envPress);
  Serial.print(",\"mood\":\""); Serial.print(MOOD_NAMES[currentMood]);
  Serial.print("\",\"comfort\":\""); Serial.print(comfortNames[comfortClass]);
  Serial.print("\",\"dewPointC\":"); if (!sensorsOk || !isfinite(dewPointC)) Serial.print("null"); else Serial.print(dewPointC, 1);
  Serial.print(",\"heatIndexC\":"); if (!sensorsOk || !isfinite(heatIndexC)) Serial.print("null"); else Serial.print(heatIndexC, 1);
  Serial.print(",\"comfortScore\":"); if (!sensorsOk || !isfinite(comfortScore)) Serial.print("null"); else Serial.print((int)comfortScore);
  Serial.print(",\"moldRisk\":"); if (!sensorsOk) Serial.print("null"); else { Serial.print('\"'); Serial.print(moldRisk); Serial.print('\"'); }
  Serial.print(",\"tempTrendCPerHr\":"); if (!sensorsOk || !isfinite(tempTrendCPerHr)) Serial.print("null"); else Serial.print(tempTrendCPerHr, 2);
  Serial.print(",\"humTrendPerHr\":"); if (!sensorsOk || !isfinite(humTrendPerHr)) Serial.print("null"); else Serial.print(humTrendPerHr, 2);
  Serial.print(",\"pressTrendHpaPerHr\":"); if (!sensorsOk || !isfinite(pressTrendHpaPerHr)) Serial.print("null"); else Serial.print(pressTrendHpaPerHr, 2);
  Serial.print(",\"pressureForecast\":\""); Serial.print(pressureForecast); Serial.print('"');
  Serial.print(",\"temperaturePrediction\":\""); Serial.print(temperaturePrediction); Serial.print('"');
  Serial.print(",\"inferred\":\""); Serial.print(inferred); Serial.print('"');
  Serial.print(",\"infMs\":"); Serial.print(infAvg, 2);
  Serial.print(",\"batt\":"); if (isnan(batteryPct)) Serial.print("null"); else Serial.print(batteryPct, 0);
  Serial.print(",\"micMute\":"); Serial.print(micMuted ? "true" : "false");
  Serial.print(",\"micRms\":"); Serial.print(micRms, 0);
  Serial.print(",\"timerMinutes\":"); Serial.print(timerDurationSec / 60UL);
  Serial.print(",\"timerRemainingSec\":"); Serial.print(timerRemainingSec);
  Serial.print(",\"timerRunning\":"); Serial.print(timerRunning ? "true" : "false");
  Serial.print(",\"timerAlarm\":"); Serial.print(isTimerAlarm ? "true" : "false");
  Serial.print(",\"page\":"); Serial.print(currentPage);
  Serial.print(",\"anomaly\":"); Serial.print(anomalyActive ? "true" : "false");
  Serial.print(",\"climate\":"); Serial.print(climateZone);
  Serial.print(",\"histT\":[");
  int n = min(historySampleCount(), (int)TELEMETRY_HISTORY_COUNT), start = historyStartIndex(n);
  for (int i = 0; i < n; i++) { int idx = historyIndex(start, i); Serial.print(histTemp[idx], 1); if (i < n - 1) Serial.print(","); }
  Serial.print("],\"histH\":[");
  for (int i = 0; i < n; i++) { int idx = historyIndex(start, i); Serial.print(histHum[idx], 1); if (i < n - 1) Serial.print(","); }
  Serial.print("],\"histP\":[");
  for (int i = 0; i < n; i++) { int idx = historyIndex(start, i); Serial.print(histPress[idx], 1); if (i < n - 1) Serial.print(","); }
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
  if (now - rawTouchChangedAt < 40UL) return;
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
        timerRunning = false; timerLock = false; timerDurationSec = TIMER_MIN_MINS * 60UL; timerRemainingSec = timerDurationSec;
        emitEvent("Timer", "Timer reset to default duration"); buzzTap();
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
          if (!timerRunning && !isTimerAlarm) {
            timerDurationSec = min(timerDurationSec + 5UL * 60UL, 120UL * 60UL);
            timerRemainingSec = timerDurationSec;
            lastInteraction = now;
            emitEvent("Timer", "Timer duration increased by 5 minutes");
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
  bool wasAlarm = isTimerAlarm;
  isTimerAlarm = false;
  timerRunning = false;
  timerLock = false;
  timerDurationSec = TIMER_MIN_MINS * 60UL;
  timerRemainingSec = timerDurationSec;
  digitalWrite(BUZZER_PIN, LOW);
  buzzQueueIdx = -1;
  buzzQueueLen = 0;
  if (wasAlarm) emitEvent("Timer", "Timer alarm dismissed");
}

bool setTimerMinutes(int minutes) {
  if (minutes < 1 || minutes > 120) return false;
  timerRunning = false;
  timerLock = false;
  isTimerAlarm = false;
  timerDurationSec = (unsigned long)minutes * 60UL;
  timerRemainingSec = timerDurationSec;
  digitalWrite(BUZZER_PIN, LOW);
  buzzQueueIdx = -1;
  buzzQueueLen = 0;
  lastInteraction = millis();
  char eventText[48];
  snprintf(eventText, sizeof(eventText), "Timer set to %d minutes", minutes);
  emitEvent("Timer", eventText);
  return true;
}

void processSerialCommands() {
  static char line[128];
  static size_t used = 0;
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch != '\n') {
      if (used + 1 < sizeof(line)) line[used++] = ch;
      else used = 0;
      continue;
    }
    line[used] = '\0'; used = 0;
    StaticJsonDocument<128> command;
    if (deserializeJson(command, line)) continue;
    const char* name = command["cmd"] | "";
    if (strcmp(name, "getEvents") == 0) {
      uint32_t afterId = command["after"] | 0UL;
      sendDeviceEventHistory(afterId);
    } else if (strcmp(name, "setTimer") == 0) {
      int minutes = command["minutes"] | 0;
      if (setTimerMinutes(minutes)) { } 
      else emitDecision("Timer must be 1 to 120 minutes");
    } else if (strcmp(name, "setTempOffset") == 0 && command["offset"].is<float>()) {
      float offset = command["offset"].as<float>();
      if (!setTemperatureOffset(offset)) emitDecision("Temperature correction must be -10 to +10 C");
    } else if (strcmp(name, "setMic") == 0 && command["enabled"].is<bool>()) {
      micMuted = !command["enabled"].as<bool>();
      emitDecision(micMuted ? "Mic muted" : "Mic level monitor enabled");
    }
  }
}

void handleButtonNav() {
  const unsigned long now = millis();
  static bool rawPressed = false;
  static unsigned long rawButtonChangedAt = 0;
  const bool sampledPressed = (digitalRead(BTN_MICMUTE_PIN) == LOW);
  if (sampledPressed != rawPressed) { rawPressed = sampledPressed; rawButtonChangedAt = now; return; }
  if (now - rawButtonChangedAt < 30UL) return;
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
        emitDecision(micMuted ? "Mic muted" : "Mic unmuted (level monitor only)");
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
          timerRunning = false; timerLock = false;
          emitEvent("Timer", "Timer paused");
          buzzTap();
        } else if (!isTimerAlarm) {
          if (timerRemainingSec == 0) timerRemainingSec = timerDurationSec;
          timerRunning = true; timerLock = true; timerLastTick = now;
          emitEvent("Timer", "Timer started");
          buzzTap();
        }
      } else {
        currentPage++;
        if (currentPage > maxMainPage()) currentPage = 0;
        if (currentPage == PAGE_INSIGHTS) triggerMiniAI();
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
  configServer.sendHeader("Access-Control-Allow-Origin", DESKPAL_CORS_ORIGIN);
  configServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  configServer.sendHeader("Access-Control-Allow-Headers", "X-DeskPal-Token, Content-Type");
  configServer.sendHeader("Access-Control-Allow-Private-Network", "true");
  configServer.sendHeader("Cache-Control", "no-store");
}

// Worst case: 60 values in each of three histories (up to 16 JSON bytes per
// float plus separators), with the remaining telemetry fields. 4096 bytes
// leaves room for the complete JSON and the MQTT packet header/topic.
// Bound: 180 history numbers at up to 16 characters each plus one comma
// apiece, and a conservative 700 bytes for all fixed fields and delimiters.
static const size_t TELEMETRY_JSON_MAX_BYTES = 60U * 3U * 17U + 700U;
static const size_t TELEMETRY_JSON_CAPACITY = 4096;
static const size_t MQTT_PACKET_CAPACITY = TELEMETRY_JSON_CAPACITY + 96U;
static_assert(TELEMETRY_JSON_CAPACITY > TELEMETRY_JSON_MAX_BYTES,
              "Telemetry JSON buffer must fit the maximum payload");
static char telemetryJsonBuffer[TELEMETRY_JSON_CAPACITY];
static StaticJsonDocument<4096> telemetryJsonDoc;

size_t buildTelemetryJson(char* out, size_t capacity) {
  telemetryJsonDoc.clear();
  JsonObject root = telemetryJsonDoc.to<JsonObject>();
  root["type"] = "telemetry";
  root["schema"] = 1;
  root["uptimeMs"] = millis();
  root["sensorsOk"] = sensorsOk;
  if (lastGoodSensorRead) root["sensorAgeMs"] = millis() - lastGoodSensorRead;
  else root["sensorAgeMs"] = nullptr;
  root["micReady"] = micReady;
  root["modelReady"] = (bool)KWS_MODEL_INTEGRATED;
  if (!sensorsOk || isnan(envTemp)) root["t"] = nullptr; else root["t"] = roundf(envTemp * 10.0f) / 10.0f;
  if (!sensorsOk || isnan(envTempRaw)) root["tRaw"] = nullptr; else root["tRaw"] = roundf(envTempRaw * 10.0f) / 10.0f;
  root["tempOffsetC"] = roundf(tempOffsetC * 10.0f) / 10.0f;
  if (!sensorsOk || isnan(envHum)) root["h"] = nullptr; else root["h"] = (int)envHum;
  if (!sensorsOk || isnan(envPress)) root["p"] = nullptr; else root["p"] = (int)envPress;
  root["mood"] = MOOD_NAMES[currentMood];
  root["comfort"] = comfortNames[comfortClass];
  if (!sensorsOk || !isfinite(dewPointC)) root["dewPointC"] = nullptr; else root["dewPointC"] = roundf(dewPointC * 10.0f) / 10.0f;
  if (!sensorsOk || !isfinite(heatIndexC)) root["heatIndexC"] = nullptr; else root["heatIndexC"] = roundf(heatIndexC * 10.0f) / 10.0f;
  if (!sensorsOk || !isfinite(comfortScore)) root["comfortScore"] = nullptr; else root["comfortScore"] = (int)comfortScore;
  if (!sensorsOk) root["moldRisk"] = nullptr; else root["moldRisk"] = moldRisk;
  if (!sensorsOk || !isfinite(tempTrendCPerHr)) root["tempTrendCPerHr"] = nullptr; else root["tempTrendCPerHr"] = roundf(tempTrendCPerHr * 100.0f) / 100.0f;
  if (!sensorsOk || !isfinite(humTrendPerHr)) root["humTrendPerHr"] = nullptr; else root["humTrendPerHr"] = roundf(humTrendPerHr * 100.0f) / 100.0f;
  if (!sensorsOk || !isfinite(pressTrendHpaPerHr)) root["pressTrendHpaPerHr"] = nullptr; else root["pressTrendHpaPerHr"] = roundf(pressTrendHpaPerHr * 100.0f) / 100.0f;
  root["pressureForecast"] = pressureForecast;
  root["temperaturePrediction"] = temperaturePrediction;
  root["inferred"] = inferred;
  root["infMs"] = roundf(infAvg * 100.0f) / 100.0f;
  if (isnan(batteryPct)) root["batt"] = nullptr; else root["batt"] = (int)roundf(batteryPct);
  root["micMute"] = micMuted;
  root["micRms"] = (int)roundf(micRms);
  root["timerMinutes"] = timerDurationSec / 60UL;
  root["timerRemainingSec"] = timerRemainingSec;
  root["timerRunning"] = timerRunning;
  root["timerAlarm"] = isTimerAlarm;
  root["page"] = currentPage;
  root["anomaly"] = anomalyActive;
  root["climate"] = climateZone;
  root["comfortLoT"] = roundf(temperatureComfort.lo() * 10.0f) / 10.0f;
  root["comfortHiT"] = roundf(temperatureComfort.hi() * 10.0f) / 10.0f;
  root["comfortLoH"] = roundf(humidityComfort.lo() * 10.0f) / 10.0f;
  root["comfortHiH"] = roundf(humidityComfort.hi() * 10.0f) / 10.0f;
  root["comfortVotes"] = temperatureComfort.votes() + humidityComfort.votes();
  root["comfortVotesT"] = temperatureComfort.votes();
  root["comfortVotesH"] = humidityComfort.votes();
  root["comfortPersonal"] = temperatureComfort.votes() >= 5 ||
                            humidityComfort.votes() >= 5;
  root["comfortPersonalT"] = temperatureComfort.votes() >= 5;
  root["comfortPersonalH"] = humidityComfort.votes() >= 5;
  const ClimateParams defaultComfort = getDefaultClimateParams();
  root["defaultLoT"] = defaultComfort.tMid - defaultComfort.tHalf;
  root["defaultHiT"] = defaultComfort.tMid + defaultComfort.tHalf;
  root["defaultLoH"] = defaultComfort.hMid - defaultComfort.hHalf;
  root["defaultHiH"] = defaultComfort.hMid + defaultComfort.hHalf;
  JsonArray histT = root.createNestedArray("histT");
  JsonArray histH = root.createNestedArray("histH");
  JsonArray histP = root.createNestedArray("histP");
  const int count = min(historySampleCount(), (int)TELEMETRY_HISTORY_COUNT);
  const int start = historyStartIndex(count);
  for (int i = 0; i < count; ++i) {
    const int idx = historyIndex(start, i);
    histT.add(roundf(histTemp[idx] * 10.0f) / 10.0f);
    histH.add(roundf(histHum[idx] * 10.0f) / 10.0f);
    histP.add(roundf(histPress[idx] * 10.0f) / 10.0f);
  }
  if (telemetryJsonDoc.overflowed()) return 0;
  const size_t length = serializeJson(telemetryJsonDoc, out, capacity);
  return length < capacity ? length : 0;
}

void sendTelemetryApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders();
    configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}");
    return;
  }
  const size_t bodyLength = buildTelemetryJson(telemetryJsonBuffer, sizeof(telemetryJsonBuffer));
  if (bodyLength == 0) {
    addApiCorsHeaders();
    configServer.send(500, "application/json", "{\"error\":\"telemetry payload overflow\"}");
    return;
  }
  addApiCorsHeaders();
  configServer.send(200, "application/json", telemetryJsonBuffer);
}

void sendEventsApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders();
    configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}");
    return;
  }
  uint32_t afterId = (uint32_t)strtoul(configServer.arg("after").c_str(), nullptr, 10);
  String body;
  body.reserve(3600);
  body = "{\"type\":\"events\",\"latestId\":";
  body += nextDeviceEventId - 1;
  uint32_t oldestId = nextDeviceEventId > DEVICE_EVENT_CAPACITY ? nextDeviceEventId - DEVICE_EVENT_CAPACITY : 1;
  uint32_t dropped = afterId + 1 < oldestId ? oldestId - afterId - 1 : 0;
  body += ",\"dropped\":"; body += dropped;
  body += ",\"events\":[";
  bool first = true;
  for (uint32_t id = oldestId; id < nextDeviceEventId; ++id) {
    if (id <= afterId) continue;
    const DeviceEvent& event = deviceEvents[(id - 1) % DEVICE_EVENT_CAPACITY];
    if (event.id != id) continue;
    if (!first) body += ',';
    first = false;
    StaticJsonDocument<256> row;
    row["id"] = event.id;
    row["uptimeMs"] = event.uptimeMs;
    row["source"] = event.source;
    row["msg"] = event.message;
    serializeJson(row, body);
  }
  body += "]}";
  addApiCorsHeaders();
  configServer.send(200, "application/json", body);
}

void handleWifiScanApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders(); configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}"); return;
  }
  int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    addApiCorsHeaders(); configServer.send(200, "application/json", "{\"state\":\"scanning\",\"networks\":[]}"); return;
  }
  if (count == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true, false);
    addApiCorsHeaders(); configServer.send(200, "application/json", "{\"state\":\"scanning\",\"networks\":[]}"); return;
  }
  StaticJsonDocument<2048> result;
  result["state"] = "complete";
  JsonArray networks = result.createNestedArray("networks");
  for (int i = 0; i < count && networks.size() < 20; ++i) {
    String name = WiFi.SSID(i);
    if (name.isEmpty()) continue;
    JsonObject row = networks.createNestedObject();
    row["ssid"] = name;
    row["rssi"] = WiFi.RSSI(i);
  }
  WiFi.scanDelete();
  String body; serializeJson(result, body);
  addApiCorsHeaders(); configServer.send(200, "application/json", body);
}

void handleWifiSaveApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders(); configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}"); return;
  }
  StaticJsonDocument<256> request;
  DeserializationError err = deserializeJson(request, configServer.arg("plain"));
  String ssid = (!err && request["ssid"].is<const char*>()) ? request["ssid"].as<String>() : "";
  String pass = (!err && request["password"].is<const char*>()) ? request["password"].as<String>() : "";
  if (ssid.isEmpty() || ssid.length() > 32 || pass.length() > 63 || (!pass.isEmpty() && pass.length() < 8)) {
    addApiCorsHeaders(); configServer.send(400, "application/json", "{\"error\":\"Choose a network name; password must be empty or 8 to 63 characters\"}"); return;
  }
  if (!prefs.begin("deskpal", false)) {
    addApiCorsHeaders(); configServer.send(500, "application/json", "{\"error\":\"Could not open saved settings\"}"); return;
  }
  bool saved = putStr("ssid", ssid) && putStr("pass", pass);
  prefs.end();
  if (!saved) {
    addApiCorsHeaders(); configServer.send(500, "application/json", "{\"error\":\"Could not save Wi-Fi settings\"}"); return;
  }
  addApiCorsHeaders();
  configServer.send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
  delay(900);
  ESP.restart();
}

void handleCalibrationApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders(); configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}"); return;
  }
  StaticJsonDocument<128> command;
  DeserializationError err = deserializeJson(command, configServer.arg("plain"));
  float offset = (!err && command["offset"].is<float>()) ? command["offset"].as<float>() : NAN;
  if (!setTemperatureOffset(offset)) {
    addApiCorsHeaders(); configServer.send(400, "application/json", "{\"error\":\"offset must be from -10 to +10 C\"}"); return;
  }
  addApiCorsHeaders();
  configServer.send(200, "application/json", String("{\"ok\":true,\"tempOffsetC\":") + String(tempOffsetC, 1) + "}");
}

void handleAlertsApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders(); configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}"); return;
  }
  StaticJsonDocument<192> request;
  DeserializationError err = deserializeJson(request, configServer.arg("plain"));
  if (err || !request["alertTmax"].is<float>() || !request["alertTmin"].is<float>() ||
      !request["alertHmax"].is<float>()) {
    addApiCorsHeaders();
    configServer.send(400, "application/json", "{\"error\":\"Provide numeric alertTmax, alertTmin, and alertHmax values\"}");
    return;
  }
  const float tmax = request["alertTmax"].as<float>();
  const float tmin = request["alertTmin"].as<float>();
  const float hmax = request["alertHmax"].as<float>();
  if (!setAlertLimits(tmax, tmin, hmax)) {
    addApiCorsHeaders();
    configServer.send(400, "application/json", "{\"error\":\"Use 0 to disable; temperature limits must be -20 to 60 C, humidity 1 to 100%, and low must be below high\"}");
    return;
  }
  addApiCorsHeaders();
  configServer.send(200, "application/json", "{\"ok\":true}");
}

void handleComfortApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders();
    configServer.send(401, "application/json",
                      "{\"error\":\"valid device token required\"}");
    return;
  }

  const String body = configServer.arg("plain");
  if (body.isEmpty() || body.length() > 128) {
    addApiCorsHeaders();
    configServer.send(400, "application/json", "{\"error\":\"invalid command\"}");
    return;
  }

  StaticJsonDocument<192> request;
  if (deserializeJson(request, body) || !request.is<JsonObject>()) {
    addApiCorsHeaders();
    configServer.send(400, "application/json", "{\"error\":\"invalid command\"}");
    return;
  }

  const int result = processComfortCommand(request.as<JsonObject>());
  if (result != COMFORT_COMMAND_OK) {
    addApiCorsHeaders();
    configServer.send(result == COMFORT_COMMAND_INVALID ? 400 : 409,
                      "application/json",
                      result == COMFORT_COMMAND_INVALID
                          ? "{\"error\":\"unsupported comfort command\"}"
                          : "{\"error\":\"comfort command rejected\"}");
    return;
  }

  publishComfortTelemetry();
  addApiCorsHeaders();
  configServer.send(200, "application/json", "{\"ok\":true}");
}

void handleTimerApi() {
  if (!tokenMatches(configServer.header("X-DeskPal-Token"))) {
    addApiCorsHeaders(); configServer.send(401, "application/json", "{\"error\":\"valid device token required\"}"); return;
  }
  StaticJsonDocument<128> command;
  DeserializationError err = deserializeJson(command, configServer.arg("plain"));
  int minutes = err ? 0 : (command["minutes"] | 0);
  if (!setTimerMinutes(minutes)) {
    addApiCorsHeaders(); configServer.send(400, "application/json", "{\"error\":\"minutes must be from 1 to 120\"}"); return;
  }
  addApiCorsHeaders();
  String response = "{\"ok\":true,\"timerMinutes\":" + String(minutes) + "}";
  configServer.send(200, "application/json", response);
}

void handleCorsOptions() {
  addApiCorsHeaders();
  configServer.send(204);
}

static unsigned long mqttRetryAt = 0, mqttLastPublish = 0;
// The public broker is unauthenticated; the pairing code acts as a shared secret.
// Accept only harmless comfort feedback commands on this topic.
static const char* MQTT_HOST = "broker.hivemq.com";
static const uint16_t MQTT_PORT = 1883;
static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 20000UL;
static const unsigned long WIFI_BACKOFF_MS[] = { 5000UL, 10000UL, 20000UL, 30000UL };
static const unsigned long MQTT_RETRY_DELAY_MS = 15000UL;

void setWifiState(WifiState nextState, const char* message) {
  if (wifiState == nextState) return;
  wifiState = nextState;
  emitEvent("Network", message);
}

void setMqttState(MqttState nextState, const char* message) {
  if (mqttState == nextState) return;
  mqttState = nextState;
  emitEvent("Network", message);
}

void startWifiConnection(unsigned long now) {
  if (!wifiWant || wifiSsid.length() == 0 || inConfigMode) return;
  wifiOn = false;
  wifiAttemptStartedAt = now;
  setWifiState(NET_WIFI_CONNECTING, "Wi-Fi connecting");
  Serial.printf("Wi-Fi attempt %u\n", (unsigned)(wifiFailureCount + 1));
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
}

void networkRestart(const char* reason) {
  if (inConfigMode || wifiSsid.length() == 0 || !wifiWant) {
    Serial.println("Network restart suppressed: portal active or no saved Wi-Fi credentials");
    return;
  }
  if (!prefs.begin("deskpal", false)) {
    Serial.println("Network restart skipped: could not save restart reason");
    return;
  }
  const bool saved = prefs.putString("restartReason", reason) > 0;
  prefs.end();
  if (!saved) {
    Serial.println("Network restart skipped: could not save restart reason");
    return;
  }
  Serial.println("Restarting after repeated network failures");
  ESP.restart();
}

void printPendingNetworkRestartReason() {
  if (!prefs.begin("deskpal", false)) return;
  String reason = prefs.getString("restartReason", "");
  if (reason.length()) {
    Serial.print("Previous network restart reason: ");
    Serial.println(reason);
    prefs.remove("restartReason");
  }
  prefs.end();
}

void handleWifiFailure(unsigned long now) {
  wifiOn = false;
  timeSynced = false;
  ++wifiFailureCount;
  Serial.printf("Wi-Fi connection failure %u/5\n", (unsigned)wifiFailureCount);
  setWifiState(NET_WIFI_BACKOFF, "Wi-Fi connection failed; backing off");
  if (wifiFailureCount >= 5) {
    networkRestart("wifi_fail_5");
    return;
  }
  const size_t delayIndex = min((size_t)(wifiFailureCount - 1),
                                sizeof(WIFI_BACKOFF_MS) / sizeof(WIFI_BACKOFF_MS[0]) - 1);
  wifiRetryAt = now + WIFI_BACKOFF_MS[delayIndex];
}

void loadPairCode() {
  static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  if (!prefs.begin("deskpal", false)) { strlcpy(pairCode, "NOSTORE1", sizeof(pairCode)); return; }
  String code = prefs.getString("pairCode", "");
  if (code.length() != 8) {
    char gen[9];
    for (int i = 0; i < 8; ++i) gen[i] = alphabet[esp_random() & 31];
    gen[8] = '\0';
    code = gen;
    prefs.putString("pairCode", code);
  }
  strlcpy(pairCode, code.c_str(), sizeof(pairCode));
  prefs.end();
}

static void mqttTopic(char* out, size_t n, const char* leaf) {
  snprintf(out, n, "deskpal/v1/%s/%s", pairCode, leaf);
}

void publishComfortTelemetry() {
  if (!wifiOn || !mqtt.connected()) return;
  const size_t bodyLength = buildTelemetryJson(telemetryJsonBuffer, sizeof(telemetryJsonBuffer));
  if (bodyLength == 0) {
    Serial.println("Warning: comfort telemetry JSON buffer overflow");
    return;
  }
  char topic[64]; mqttTopic(topic, sizeof(topic), "telemetry");
  if (!mqtt.publish(topic, (const uint8_t*)telemetryJsonBuffer, bodyLength, false))
    Serial.println("Warning: comfort telemetry publish failed");
}

void mqttComfortCommand(char* topic, byte* payload, unsigned int length) {
  (void)topic;
  if (!payload || length == 0 || length > 128) return;
  char json[129];
  memcpy(json, payload, length);
  json[length] = '\0';
  StaticJsonDocument<192> request;
  if (deserializeJson(request, json, length) || !request.is<JsonObject>()) return;
  if (processComfortCommand(request.as<JsonObject>()) == COMFORT_COMMAND_OK)
    publishComfortTelemetry();
}

void mqttPublishEvent(const DeviceEvent& e) {
  if (!wifiOn || !mqtt.connected()) return;
  char topic[64]; mqttTopic(topic, sizeof(topic), "events");
  StaticJsonDocument<256> doc;
  doc["type"] = "decision"; doc["id"] = e.id; doc["uptimeMs"] = e.uptimeMs;
  doc["source"] = e.source; doc["msg"] = e.message;
  char buf[256]; size_t n = serializeJson(doc, buf, sizeof(buf));
  if (!mqtt.publish(topic, (const uint8_t*)buf, n, false)) Serial.println("Warning: MQTT event publish failed");
}

void mqttTick(unsigned long now) {
  if (!wifiOn) return;
  if (!mqtt.connected()) {
    if (now < mqttRetryAt) return;
    mqttRetryAt = now + 15000UL;
    char cid[32]; snprintf(cid, sizeof(cid), "dp-%s-%04x", pairCode, (unsigned)(esp_random() & 0xFFFF));
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setBufferSize(MQTT_PACKET_CAPACITY);
    mqtt.setSocketTimeout(2);
    mqtt.setCallback(mqttComfortCommand);
    if (!mqtt.connect(cid)) return;
    Serial.println("Cloud link: connected");
    char commandTopic[64]; mqttTopic(commandTopic, sizeof(commandTopic), "cmd");
    if (!mqtt.subscribe(commandTopic)) Serial.println("Warning: MQTT comfort command subscription failed");
    mqttLastPublish = 0;
  }
  mqtt.loop();
  if (now - mqttLastPublish >= 2000UL) {
    mqttLastPublish = now;
    char topic[64]; mqttTopic(topic, sizeof(topic), "telemetry");
    const size_t bodyLength = buildTelemetryJson(telemetryJsonBuffer, sizeof(telemetryJsonBuffer));
    if (bodyLength == 0) Serial.println("Warning: telemetry JSON buffer overflow");
    else if (!mqtt.publish(topic, (const uint8_t*)telemetryJsonBuffer, bodyLength, false))
      Serial.println("Warning: MQTT telemetry publish failed");
  }
}

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
  if (WiFi.status() != WL_CONNECTED) return;
  
  WeatherSnapshot next = getWeatherSnapshot();
  char cityQ[193];
  trimUrlPart(city, cityQ, sizeof(cityQ));
  if (cityQ[0] == '\0' || apiKey.isEmpty()) {
    next.ok = false;
    next.weatherHttp = -100; 
    publishWeatherSnapshot(next);
    return;
  }
  char cc[16]; trimUrlPart(countryCode, cc, sizeof(cc));
  bool useCountry = (strlen(cc) == 2);

  char urlBuf[640];
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  buildOwmUrl(urlBuf, sizeof(urlBuf), "weather", cityQ, useCountry);
  if (!http.begin(urlBuf)) {
    next.weatherHttp = HTTPC_ERROR_CONNECTION_REFUSED; 
    next.ok = false;
    publishWeatherSnapshot(next); 
    return;
  }
  
  int httpCode = http.GET();
  if (httpCode == 404 && useCountry) {
    http.end(); useCountry = false;
    buildOwmUrl(urlBuf, sizeof(urlBuf), "weather", cityQ, false);
    if (!http.begin(urlBuf)) {
      next.weatherHttp = HTTPC_ERROR_CONNECTION_REFUSED; 
      next.ok = false;
      publishWeatherSnapshot(next); 
      return;
    }
    httpCode = http.GET();
  }
  
  if (httpCode != HTTP_CODE_OK) { 
    Serial.print("Weather HTTP "); Serial.print(httpCode); Serial.print(" "); Serial.println(HTTPClient::errorToString(httpCode)); 
  }
  
  bool weatherParsed = false;
  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, http.getStream());
    if (!err && doc["main"]["temp"].is<float>() && doc["main"]["humidity"].is<int>()) {
      next.temperature = doc["main"]["temp"].as<float>();
      next.feelsLike = doc["main"]["feels_like"].as<float>();
      next.humidity = doc["main"]["humidity"].as<int>();
      next.timezoneOffset = doc["timezone"] | 0L;
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
    next.weatherHttp = httpCode;
    publishWeatherSnapshot(next);
    return;
  }

  if (ESP.getFreeHeap() < 50000) {
    next.forecastHttp = HTTPC_ERROR_TOO_LESS_RAM;
    publishWeatherSnapshot(next);
    return;
  }
  for (int i = 0; i < 3; ++i) next.forecast[i] = {};
  buildOwmUrl(urlBuf, sizeof(urlBuf), "forecast", cityQ, useCountry);
  if (!http.begin(urlBuf)) {
    next.forecastHttp = HTTPC_ERROR_CONNECTION_REFUSED;
    publishWeatherSnapshot(next); return;
  }
  httpCode = http.GET();
  next.forecastHttp = httpCode;
  if (httpCode == HTTP_CODE_OK) {
    StaticJsonDocument<192> filter;
    filter["city"]["timezone"] = true;
    filter["list"][0]["dt"] = true;
    filter["list"][0]["main"]["temp"] = true;
    filter["list"][0]["weather"][0]["main"] = true;
    DynamicJsonDocument fdoc(6144);
    DeserializationError err = deserializeJson(fdoc, http.getStream(), DeserializationOption::Filter(filter));
    if (!err) {
      const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
      time_t utcNow; time(&utcNow);
      time_t todayEpoch = utcNow + next.timezoneOffset;
      struct tm todayLocal;
      gmtime_r(&todayEpoch, &todayLocal);
      int todayKey = (todayLocal.tm_year + 1900) * 10000 + (todayLocal.tm_mon + 1) * 100 + todayLocal.tm_mday;
      int dayKeys[3] = { 0, 0, 0 }, dayWeekdays[3] = { 0, 0, 0 };
      int bestDistance[3] = { 2147483647, 2147483647, 2147483647 };
      JsonArray list = fdoc["list"].as<JsonArray>();
      for (JsonObject item : list) {
        if (!item["dt"].is<long>() || !item["main"]["temp"].is<float>()) continue;
        time_t localEpoch = (time_t)item["dt"].as<long>() + next.timezoneOffset;
        struct tm localForecast;
        gmtime_r(&localEpoch, &localForecast);
        int dayKey = (localForecast.tm_year + 1900) * 10000 + (localForecast.tm_mon + 1) * 100 + localForecast.tm_mday;
        if (dayKey == todayKey) continue;
        int slot = -1;
        for (int i = 0; i < 3; ++i) if (dayKeys[i] == dayKey) { slot = i; break; }
        if (slot < 0) for (int i = 0; i < 3; ++i) if (dayKeys[i] == 0) { slot = i; dayKeys[i] = dayKey; dayWeekdays[i] = localForecast.tm_wday; break; }
        if (slot < 0) continue;
        int localSeconds = localForecast.tm_hour * 3600 + localForecast.tm_min * 60 + localForecast.tm_sec;
        int distance = abs(localSeconds - 43200);
        if (distance >= bestDistance[slot]) continue;
        bestDistance[slot] = distance;
        next.forecast[slot].temp = (int)lroundf(item["main"]["temp"].as<float>());
        const char* icon = item["weather"][0]["main"];
        if (icon) strlcpy(next.forecast[slot].iconType, icon, sizeof(next.forecast[slot].iconType));
        strlcpy(next.forecast[slot].dayName, days[dayWeekdays[slot]], sizeof(next.forecast[slot].dayName));
      }
    }
  }
  http.end();
  publishWeatherSnapshot(next);
}

void wifiTick(unsigned long now) {
  if (!wifiWant || wifiSsid.length() == 0 || inConfigMode) {
    wifiOn = false;
    setWifiState(NET_WIFI_IDLE, "Wi-Fi idle");
    return;
  }

  if (wifiState == NET_WIFI_IDLE) startWifiConnection(now);
  else if (wifiState == NET_WIFI_BACKOFF && now >= wifiRetryAt) startWifiConnection(now);

  const wl_status_t status = WiFi.status();
  if (wifiState == NET_WIFI_CONNECTING) {
    if (status == WL_CONNECTED) {
      wifiOn = true;
      wifiFailureCount = 0;
      mqttFailureCount = 0;
      wifiRetryAt = 0;
      mqttRetryAt = 0;
      setWifiState(NET_WIFI_CONNECTED, "Wi-Fi connected");
      // Request UTC from NTP, then apply the configured POSIX timezone locally.
      // configTime() may reset TZ, so set it after every configTime() call.
      configTime(0, 0, ntpServer, "time.google.com", "time.cloudflare.com");
      setenv("TZ", tzString.c_str(), 1);
      tzset();
      timeSynced = false;

      if (!telemetryServerStarted) {
        const char* headerNames[] = { "X-DeskPal-Token" };
        configServer.collectHeaders(headerNames, 1);
        configServer.on("/api/telemetry", HTTP_GET, sendTelemetryApi);
        configServer.on("/api/telemetry", HTTP_OPTIONS, handleCorsOptions);
        configServer.on("/api/events", HTTP_GET, sendEventsApi);
        configServer.on("/api/events", HTTP_OPTIONS, handleCorsOptions);
        configServer.on("/api/wifi/scan", HTTP_GET, handleWifiScanApi);
        configServer.on("/api/wifi/scan", HTTP_OPTIONS, handleCorsOptions);
        configServer.on("/api/wifi", HTTP_POST, handleWifiSaveApi);
        configServer.on("/api/wifi", HTTP_OPTIONS, handleCorsOptions);
        configServer.on("/api/timer", HTTP_POST, handleTimerApi);
        configServer.on("/api/timer", HTTP_OPTIONS, handleCorsOptions);
        configServer.on("/api/calibration", HTTP_POST, handleCalibrationApi);
        configServer.on("/api/calibration", HTTP_OPTIONS, handleCorsOptions);
        configServer.on("/api/alerts", HTTP_POST, handleAlertsApi);
        configServer.on("/api/alerts", HTTP_OPTIONS, handleCorsOptions);
  configServer.on("/api/comfort", HTTP_POST, handleComfortApi);
  configServer.on("/api/comfort", HTTP_OPTIONS, handleCorsOptions);
        configServer.onNotFound([]() {
          configServer.sendHeader("Access-Control-Allow-Origin", DESKPAL_CORS_ORIGIN);
          configServer.send(404, "application/json", "{\"error\":\"not found\"}");
        });
        MDNS.begin("deskpal");
        configServer.begin(); telemetryServerStarted = true;
        Serial.print("Telemetry API: http://"); Serial.print(WiFi.localIP()); Serial.println("/api/telemetry");
        Serial.print("Telemetry token: "); Serial.println(telemetryToken);
        Serial.print("DASHBOARD PAIRING CODE: "); Serial.println(pairCode);
      }
      lastWeatherUpdate = 0;
      Serial.println("Wi-Fi connected; waiting for NTP");
    } else if (status == WL_CONNECT_FAILED ||
               now - wifiAttemptStartedAt >= WIFI_CONNECT_TIMEOUT_MS) {
      handleWifiFailure(now);
    }
  } else if (wifiState == NET_WIFI_CONNECTED && status != WL_CONNECTED) {
    Serial.println("Wi-Fi link lost");
    handleWifiFailure(now);
  }

  if (wifiOn && !timeSynced) {
    time_t epoch = 0; time(&epoch);
    if (epoch >= 1700000000) {
      timeSynced = true;
      lastWeatherUpdate = 0;
      Serial.println("NTP time synchronized");
      emitEvent("Network", "Clock synchronized");
    } else {
      static unsigned long lastNtpAttempt = 0;
      if (now - lastNtpAttempt > 15000) {
        lastNtpAttempt = now;
        configTime(0, 0, ntpServer, "time.google.com", "time.cloudflare.com");
        setenv("TZ", tzString.c_str(), 1);
        tzset();
        Serial.println("NTP: Retrying time sync...");
      }
    }
  }
}
#endif

bool currentTm(struct tm* t) {
  if (timeSynced) {
    time_t epoch; time(&epoch);
    return localtime_r(&epoch, t) != nullptr;
  }
  time_t demo = demoBaseEpoch + millis() / 1000;
  return gmtime_r(&demo, t) != nullptr;
}

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
  drawHeader("ROOM"); display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (!sensorsOk) {
    drawFitLine(sensorFailCount > 0 && sensorFailCount < SENSOR_RECOVERY_FAILS ? "Sensor recovering..." : "No sensor data", 29);
    drawFitLine("Check BME280 wiring", 42);
    return;
  }
  char line[32];
  snprintf(line, sizeof(line), "%.1f C", envTemp); drawFitLine(line, 24, 14);
  snprintf(line, sizeof(line), "Humidity  %d%%", (int)envHum); drawFitLine(line, 42, 18);

  const int sampleCount = min(historySampleCount(), (int)TELEMETRY_HISTORY_COUNT);
  if (sampleCount < 5) return;
  const int historyStart = historyStartIndex(sampleCount);

  const int graphX = 64, graphY = 50, graphWidth = 60, graphHeight = 14;
  float minTemp = INFINITY, maxTemp = -INFINITY;
  for (int i = 0; i < sampleCount; ++i) {
    const int idx = historyIndex(historyStart, i);
    minTemp = fminf(minTemp, histTemp[idx]);
    maxTemp = fmaxf(maxTemp, histTemp[idx]);
  }
  if (maxTemp - minTemp < 1.0f) {
    const float center = (minTemp + maxTemp) * 0.5f;
    minTemp = center - 0.5f;
    maxTemp = center + 0.5f;
  }

  int previousX = graphX, previousY = graphY + graphHeight - 1;
  for (int i = 0; i < sampleCount; ++i) {
    const int idx = historyIndex(historyStart, i);
    const int x = graphX + i * (graphWidth - 1) / (sampleCount - 1);
    const float normalized = (histTemp[idx] - minTemp) / (maxTemp - minTemp);
    const int y = graphY + graphHeight - 1 - (int)lroundf(normalized * (graphHeight - 1));
    if (i > 0) display.drawLine(previousX, previousY, x, y, OLED_WHITE);
    previousX = x;
    previousY = y;
  }
}
void drawInsightsPage() {
  drawHeader("INSIGHTS");
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  const unsigned long panel = (millis() / 5000UL) % 4UL;
  if (panel == 1) {
    const bool learned = temperatureComfort.votes() >= 5;
    const ClimateParams defaults = getDefaultClimateParams();
    const float lo = learned ? temperatureComfort.lo() : defaults.tMid - defaults.tHalf;
    const float hi = learned ? temperatureComfort.hi() : defaults.tMid + defaults.tHalf;
    drawFitLine(learned ? "Temperature estimate" : "Default temp range", 21);
    char rangeLine[32];
    snprintf(rangeLine, sizeof(rangeLine), learned ? "Your range: %.0f-%.0fC" : "Default: %.0f-%.0fC", lo, hi);
    drawFitLine(rangeLine, 34);
    char votesLine[32];
    if (learned) snprintf(votesLine, sizeof(votesLine), "Votes: %lu", (unsigned long)temperatureComfort.votes());
    else snprintf(votesLine, sizeof(votesLine), "Votes: %lu (learning)", (unsigned long)temperatureComfort.votes());
    drawFitLine(votesLine, 46);
    drawFitLine(learned ? "Range is an estimate" : "Rate it on dashboard", 57);
    return;
  }
  if (panel == 2) {
    const bool learned = humidityComfort.votes() >= 5;
    const ClimateParams defaults = getDefaultClimateParams();
    const float lo = learned ? humidityComfort.lo() : defaults.hMid - defaults.hHalf;
    const float hi = learned ? humidityComfort.hi() : defaults.hMid + defaults.hHalf;
    drawFitLine(learned ? "Humidity estimate" : "Default humidity", 21);
    char rangeLine[32];
    snprintf(rangeLine, sizeof(rangeLine), learned ? "Your range: %.0f-%.0f%%" : "Default: %.0f-%.0f%%", lo, hi);
    drawFitLine(rangeLine, 34);
    char votesLine[32];
    if (learned) snprintf(votesLine, sizeof(votesLine), "Votes: %lu", (unsigned long)humidityComfort.votes());
    else snprintf(votesLine, sizeof(votesLine), "Votes: %lu (learning)", (unsigned long)humidityComfort.votes());
    drawFitLine(votesLine, 46);
    drawFitLine(learned ? "Range is an estimate" : "Rate it on dashboard", 57);
    return;
  }
  if (panel == 3) {
    drawFitLine("Forecast estimate", 22);
    drawFitLine(pressureForecast, 34);
    drawFitLine(temperaturePrediction, 48);
    return;
  }
  drawFitLine(insightTitle, 19);
  char scoreLine[24];
  if (sensorsOk && isfinite(comfortScore)) snprintf(scoreLine, sizeof(scoreLine), "Comfort estimate: %d", (int)comfortScore);
  else strlcpy(scoreLine, "Comfort estimate: --", sizeof(scoreLine));
  drawFitLine(scoreLine, 29);
  drawFitLine("Dew point estimate", 38);
  char dewLine[24];
  if (sensorsOk && isfinite(dewPointC)) snprintf(dewLine, sizeof(dewLine), "%.1f C", dewPointC);
  else strlcpy(dewLine, "--", sizeof(dewLine));
  drawFitLine(dewLine, 47);
  drawFitLine(insightAction, 57);
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
    char eb[48];
    if (weather.weatherHttp == -100) snprintf(eb, sizeof(eb), "No city/API key set");
    else if (weather.weatherHttp == 401) snprintf(eb, sizeof(eb), "401 bad API key");
    else if (weather.weatherHttp == 404) snprintf(eb, sizeof(eb), "404 city not found");
    else if (weather.weatherHttp == 429) snprintf(eb, sizeof(eb), "429 call limit");
    else if (weather.weatherHttp == HTTP_CODE_OK) snprintf(eb, sizeof(eb), "Data unreadable");
    else if (weather.weatherHttp == HTTPC_ERROR_CONNECTION_REFUSED) snprintf(eb, sizeof(eb), "Connection refused");
    else if (weather.weatherHttp != 0) snprintf(eb, sizeof(eb), "HTTP %d failed", weather.weatherHttp);
    else if (!timeSynced) snprintf(eb, sizeof(eb), "Waiting for time sync");
    else snprintf(eb, sizeof(eb), "Waiting for weather...");
    drawFitLine(eb, 36);
    drawFitLine("Check WiFi, city, API key", 48);
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
  drawHeader("NEXT 3 DAYS"); display.setTextColor(OLED_WHITE);
  WeatherSnapshot weather = getWeatherSnapshot();
  if (!wifiOn) { drawFitLine("WiFi not connected", 31); return; }
  if (weather.forecast[0].dayName[0] == '\0') {
    display.setFont(NULL);
    char eb[32];
    if (weather.forecastHttp != 0 && weather.forecastHttp != 200) snprintf(eb, sizeof(eb), "Forecast HTTP %d", weather.forecastHttp);
    else snprintf(eb, sizeof(eb), weather.ok ? "Forecast loading" : "Waiting for weather");
    drawFitLine(eb, 28);
    drawFitLine("Check WEATHER page", 42);
    return;
  }
  display.drawLine(42, 18, 42, 61, OLED_WHITE); display.drawLine(85, 18, 85, 61, OLED_WHITE);
  for (int i = 0; i < 3; i++) {
    int centerX = i * 43 + 21;
    display.setFont(NULL);
    const char* d = weather.forecast[i].dayName;
    display.setCursor(centerX - (int)(strlen(d) * 3), 18); display.print(d);
    display.drawBitmap(centerX - 8, 29, getMiniIcon(weather.forecast[i].iconType), 16, 16, OLED_WHITE);
    char tBuf[8]; snprintf(tBuf, sizeof(tBuf), "%d C", weather.forecast[i].temp);
    display.setCursor(centerX - (int)(strlen(tBuf) * 3), 50); display.print(tBuf);
  }
}
#endif
void drawTimerPage() {
  drawHeader("TIMER");
  int mins = timerRemainingSec / 60, secs = timerRemainingSec % 60;
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mins, secs);
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  const char* state = isTimerAlarm ? "TIME UP" : timerRunning ? "RUNNING" :
                      (timerRemainingSec < timerDurationSec ? "PAUSED" : "READY");
  display.setCursor(64 - (int)(strlen(state) * 3), 23); display.print(state);
  display.setFont(&FreeSansBold18pt7b);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(max(0, (SCREEN_WIDTH - (int)w) / 2), 57); display.print(timeBuf);
  display.setFont(NULL);
}

void drawTimerOverlay() {
  if (!timerRunning || currentPage == PAGE_TIMER) return;
  int mins = timerRemainingSec / 60, secs = timerRemainingSec % 60;
  char buf[8]; snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
  int w = 58, h = 12;
  int x = SCREEN_WIDTH - w - 2, y = 2;
  display.fillRect(x, y, w, h, OLED_BLACK); display.drawRect(x, y, w, h, OLED_WHITE);
  display.setFont(NULL); display.setTextColor(OLED_WHITE); display.setCursor(x + 3, y + 2); display.print("RUN "); display.print(buf);
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
  initializeComfortLearning();
#if ENABLE_WEATHER
  printPendingNetworkRestartReason();
  loadTelemetryToken();
  loadPairCode();
#endif
#if ENABLE_WEATHER
  if (forceConfig) {
    bmpOk = bme.begin() || bme.begin(0x76);
    Serial.println(bmpOk ? "BME280 OK" : "BME280 NOT FOUND - check wiring!");
    startConfigPortal(); return;
  }
#endif
  leftEye.init(18, 14, 36, 36); rightEye.init(74, 14, 36, 36);
  playBootAnimation();
  bmpOk = bme.begin() || bme.begin(0x76);
  Serial.println(bmpOk ? "BME280 OK" : "BME280 NOT FOUND - check wiring!");
  if (bmpOk) {
    for (int i = 0; i < 5; i++) { bme.readTemperature(); bme.readHumidity(); bme.readPressure(); delay(40); }
    Serial.println("=== TEMP CALIBRATION ===");
    Serial.print("Raw: "); Serial.println(bme.readTemperature());
    Serial.print("Adj: "); Serial.println(bme.readTemperature() + tempOffsetC);
  }
  lastInteraction = millis(); readSensors(); generateInsights(); batteryPct = readBatteryPct();
  lastSensorRead = millis(); lastInsightGen = millis(); lastPageSwitch = millis();
  initMic();
#if ENABLE_WEATHER
  if (wifiSsid.length() > 0) {
    wifiWant = true; WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.setAutoReconnect(false); WiFi.persistent(false);
    startWifiConnection(millis());
    Serial.println("Wi-Fi connecting in background");
  } else { Serial.println("WiFi: no credentials saved (hold touch @ boot to configure)"); }
#endif
  buzzBoot(); emitDecision("Boot complete");
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
        emitEvent("Timer", "Timer finished");
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
  processSerialCommands();
  if (inConfigMode) {
#if ENABLE_WEATHER
    configServer.handleClient();
#endif
    unsigned long configNow = millis();
    if (configNow - lastSensorRead > 2000) { readSensors(); lastSensorRead = configNow; }
    if (configNow - lastInsightGen > 10000) { generateInsights(); lastInsightGen = configNow; }
    updateTimer(); updateBuzzer(); checkAlertThresholds(); computeMood(); emitDashboardTelemetry();
    return;
  }
  unsigned long now = millis();
  static unsigned long lastMiniAIIntervalAt = 0;
  if ((currentPage == PAGE_INSIGHTS || currentPage == PAGE_EYES) &&
      now - lastMiniAIIntervalAt >= 60000UL) {
    lastMiniAIIntervalAt = now;
    triggerMiniAI();
  }
  if (OLED_TIMEOUT_MS > 0 && !screenOff && now - lastInteraction > OLED_TIMEOUT_MS) { DISPLAY_OFF(); screenOff = true; }
  handleInputs(); liftCheck(); processAudio(); updateTimer(); updateBuzzer();
#if ENABLE_WEATHER
  wifiTick(now);
  mqttTick(now);
  if (telemetryServerStarted) configServer.handleClient();
#endif
  if (now - lastSensorRead > 2000) { readSensors(); lastSensorRead = now; }
  if (now - lastInsightGen > 10000) { generateInsights(); lastInsightGen = now; }
  checkAlertThresholds();
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
    case PAGE_TIMER: drawTimerPage(); break;
  }
  drawTimerOverlay();
  display.display();
}
