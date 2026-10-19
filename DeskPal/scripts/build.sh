#!/usr/bin/env sh
set -eu

arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
arduino-cli lib install 'Adafruit BME280 Library@2.2.4'
arduino-cli lib install 'Adafruit BusIO@1.16.2'
arduino-cli lib install 'Adafruit GFX Library@1.11.11'
arduino-cli lib install 'Adafruit SH110X@2.1.12'
arduino-cli lib install 'Adafruit SSD1306@2.5.13'
arduino-cli lib install 'Adafruit Unified Sensor@1.1.15'
arduino-cli lib install 'ArduinoJson@6.21.5'
arduino-cli compile --profile wroom32 .
