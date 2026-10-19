#pragma once

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
#include "DeskPalTrust.h"
#endif

#include <Adafruit_BME280.h>
#include "DeskPalAssets.h"

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
#include "DeskPalKwsAdapter.h"
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
#define PET_LIMIT 5
#define PET_WINDOW_MS 6000UL
#define RUB_WINDOW_MS 1200UL
#define RUB_MIN_TRANSITIONS 4
#define BATT_LOW_PCT 15

#define BTN_LONG_PRESS_MS        1500UL
#define BTN_VERY_LONG_PRESS_MS   5000UL
#define TOUCH_LONG_HOLD_MS       1500UL
#define OLED_TIMEOUT_MS          300000UL
#define DOUBLE_TAP_DELAY_MS      350UL
#define TIMER_MIN_MINS           10

// Maps the region comfort range edges to the MLP switching point.
#define COMFORT_EDGE_SCALE 0.45f

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

