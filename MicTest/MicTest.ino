// Standalone INMP441 microphone diagnostic for the classic ESP32.
// Upload this sketch from the MicTest folder; it does not include DeskPal firmware.
// Wiring: VDD=3V3, GND=GND, L/R=GND, SCK=GPIO18, WS=GPIO19, SD=GPIO35.

#include <Arduino.h>
#include <math.h>
#include "driver/i2s.h"

static const i2s_port_t MIC_PORT = I2S_NUM_0;
static const int MIC_SCK = 18;
static const int MIC_WS = 19;
static const int MIC_SD = 35;
static const uint32_t SAMPLE_RATE = 16000;
static const size_t SAMPLE_COUNT = 512;

static int32_t samples[SAMPLE_COUNT];
static bool micStarted = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("INMP441 standalone test");
  Serial.println("Expected: SCK=18 WS=19 SD=35 L/R=GND");

  i2s_config_t config = {};
  config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  config.sample_rate = SAMPLE_RATE;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT; // INMP441 L/R is tied to GND
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = SAMPLE_COUNT;
  config.use_apll = false;

  esp_err_t err = i2s_driver_install(MIC_PORT, &config, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("I2S driver install failed: %d\n", (int)err);
    return;
  }

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = MIC_SCK;
  pins.ws_io_num = MIC_WS;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = MIC_SD;

  err = i2s_set_pin(MIC_PORT, &pins);
  if (err != ESP_OK) {
    Serial.printf("I2S pin setup failed: %d\n", (int)err);
    i2s_driver_uninstall(MIC_PORT);
    return;
  }

  i2s_zero_dma_buffer(MIC_PORT);
  micStarted = true;
  Serial.println("I2S ready. Stay quiet, then clap or speak near the mic.");
}

void loop() {
  if (!micStarted) {
    delay(1000);
    return;
  }
  size_t bytesRead = 0;
  const esp_err_t err = i2s_read(MIC_PORT, samples, sizeof(samples), &bytesRead,
                                 pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    Serial.printf("I2S read failed: %d\n", (int)err);
    delay(250);
    return;
  }
  if (bytesRead == 0) {
    Serial.println("I2S read returned 0 bytes");
    delay(250);
    return;
  }

  const size_t count = bytesRead / sizeof(samples[0]);
  int32_t minSample = INT32_MAX;
  int32_t maxSample = INT32_MIN;
  float mean = 0.0f;
  for (size_t i = 0; i < count; ++i) {
    const int32_t sample24 = samples[i] >> 8; // INMP441 24-bit sample, left-aligned in 32 bits
    if (sample24 < minSample) minSample = sample24;
    if (sample24 > maxSample) maxSample = sample24;
    mean += (float)sample24;
  }
  mean /= (float)count;

  float sumSquares = 0.0f;
  for (size_t i = 0; i < count; ++i) {
    const float centered = (float)(samples[i] >> 8) - mean;
    sumSquares += centered * centered;
  }
  const float rms = sqrtf(sumSquares / (float)count);

  Serial.printf("bytes=%u samples=%u min=%ld max=%ld rms=%.0f\n",
                (unsigned)bytesRead, (unsigned)count,
                (long)minSample, (long)maxSample, rms);
}
