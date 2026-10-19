#pragma once

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
  if (commandSyllables >= 2) {
    timerDurationSec = (timerRemainingSec > 0) ? timerDurationSec : TIMER_MIN_MINS * 60;
    if (timerRemainingSec == 0) timerRemainingSec = timerDurationSec;
    timerRunning = true; timerLock = true; timerLastTick = millis();
    snprintf(aiResponse, sizeof(aiResponse), "Timer started: %lu min. Focus!", timerRemainingSec / 60);
    buzzHappy();
  } else if (commandSyllables == 1) {
    if (timerRunning) {
      timerRunning = false; timerLock = false;
      snprintf(aiResponse, sizeof(aiResponse), "Timer stopped at %lu min.", timerRemainingSec / 60);
      buzzTap();
    } else {
      strlcpy(aiResponse, "No timer running to stop!", sizeof(aiResponse));
      buzzTap();
    }
  } else { triggerMiniAI(); }
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
  float rms = readMicRms();
#if USE_TFLITE
  int label = classifyKeywordEdge();
  handleKeywordLabel(label);
  return;
#endif
  if (detectHeyPalEnergy(rms)) keywordSpotted();
}
