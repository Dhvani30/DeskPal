#pragma once

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
  display.setTextColor(OLED_WHITE); display.setFont(NULL); display.setTextSize(1);
  if (!timeSynced) { display.setCursor(0, 0); display.print("DEMO"); }
  drawWifiIcon(2, 0, wifiOn); drawBatteryIcon(106, 0);
  display.setFont(&FreeSansBold18pt7b);
  char timeStr[6]; sprintf(timeStr, "%02d:%02d", t.tm_hour, t.tm_min);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int)w) / 2, 42); display.print(timeStr);
  display.setFont(&FreeSans9pt7b);
  char dateStr[20]; strftime(dateStr, 20, "%a, %b %d", &t);
  display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - (int)w) / 2, 62); display.print(dateStr);
}
void drawEnvPage() {
  drawHeader("ENVIRONMENT"); display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (!sensorsOk) {
    display.setCursor(6, 26);
    if (sensorFailCount > 0 && sensorFailCount < SENSOR_RECOVERY_FAILS) display.print("Sensor recovering...");
    else display.print("No sensor data");
    return;
  }
  char buf[24];
  snprintf(buf, sizeof(buf), "T:%.1fC", envTemp); display.setCursor(4, 24); display.print(buf);
  snprintf(buf, sizeof(buf), "H:%d%%", (int)envHum); display.setCursor(70, 24); display.print(buf);
  snprintf(buf, sizeof(buf), "P:%dhPa", (int)envPress); display.setCursor(4, 36); display.print(buf);
  snprintf(buf, sizeof(buf), "A:%dm", (int)envAlt); display.setCursor(70, 36); display.print(buf);
  display.drawLine(0, 46, 128, 46, OLED_WHITE);
  const char* c = comfortNames[comfortClass];
  display.setCursor(64 - (int)(strlen(c) * 3), 48); display.print(c);
  display.setCursor(2, 56); display.print("Inf:"); display.print(infAvg, 1); display.print("ms");
  if (anomalyActive) drawWarnIcon(112, 50);
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
  drawHeader("VOICE DEMO");
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
    else { display.setCursor(4, 54); display.print("Wake model demo only"); }
  }
}
#if ENABLE_WEATHER
void drawWeatherCard() {
  drawHeader("WEATHER");
  WeatherSnapshot weather = getWeatherSnapshot();
  if (!wifiOn) { display.setFont(NULL); display.setCursor(0, 22); display.print("No WiFi yet...\nretrying auto.\nHold touch @boot\nto reconfigure."); return; }
  if (city.length() == 0 || apiKey.length() == 0) { display.setFont(NULL); display.setCursor(0, 24); display.print("Set City + API Key\nin config portal"); return; }
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (!weather.ok) {
    display.setCursor(0, 24); display.print(cityUpper);
    char eb[32];
    if (weather.weatherHttp == 401) snprintf(eb, sizeof(eb), "401 bad API key");
    else if (weather.weatherHttp == 404) snprintf(eb, sizeof(eb), "404 city not found");
    else if (weather.weatherHttp == 429) snprintf(eb, sizeof(eb), "429 call limit");
    else if (weather.weatherHttp != 0) snprintf(eb, sizeof(eb), "HTTP %d failed", weather.weatherHttp);
    else snprintf(eb, sizeof(eb), "First fetch pending");
    display.setCursor(0, 36); display.print(eb);
    display.setCursor(0, 48); display.print("Retry every 60 s.");
    display.setCursor(0, 58); display.print("See serial log.");
    return;
  }
  display.drawBitmap(96, 16, getBigIcon(weather.main), 32, 32, OLED_WHITE);
  display.setFont(NULL);
  display.setCursor(0, 21); display.print(cityUpper);
  display.setFont(&FreeSansBold18pt7b);
  int tempInt = (int)weather.temperature;
  display.setCursor(0, 52); display.print(tempInt);
  int16_t x1, y1; uint16_t w, h;
  char tmpBuf[6]; snprintf(tmpBuf, sizeof(tmpBuf), "%d", tempInt);
  display.getTextBounds(tmpBuf, 0, 52, &x1, &y1, &w, &h);
  display.fillCircle(x1 + w + 5, 34, 4, OLED_WHITE);
  display.setFont(NULL);
  display.drawBitmap(88, 36, bmp_tiny_drop, 8, 8, OLED_WHITE);
  display.setCursor(100, 36); display.print(weather.humidity); display.print("%");
  display.setCursor(88, 48); display.print("~"); display.print((int)weather.feelsLike);
  display.drawLine(0, 56, 128, 56, OLED_WHITE);
  display.setCursor(0, 55); display.print(weather.description);
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
  display.drawLine(42, 16, 42, 64, OLED_WHITE); display.drawLine(85, 16, 85, 64, OLED_WHITE);
  for (int i = 0; i < 3; i++) {
    int centerX = i * 43 + 21;
    display.setFont(NULL);
    const char* d = weather.forecast[i].dayName;
    display.setCursor(centerX - (int)(strlen(d) * 3), 20); display.print(d);
    display.drawBitmap(centerX - 8, 28, getMiniIcon(weather.forecast[i].iconType), 16, 16, OLED_WHITE);
    display.setFont(&FreeSansBold9pt7b);
    int16_t x1, y1; uint16_t w, h;
    char tBuf[6]; snprintf(tBuf, sizeof(tBuf), "%d", weather.forecast[i].temp);
    display.getTextBounds(tBuf, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(centerX - (int)(w / 2) - 2, 60); display.print(weather.forecast[i].temp);
    display.fillCircle(centerX + (int)(w / 2) + 1, 52, 2, OLED_WHITE);
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
  drawHeader("WORLD CLOCK"); display.drawLine(64, 18, 64, 54, OLED_WHITE);
  display.setFont(NULL); display.setCursor(16, 22); display.print("INDIA");
  display.setFont(&FreeSansBold9pt7b); char iStr[10]; sprintf(iStr, "%02d:%02d", i_h, i_m); display.setCursor(5, 46); display.print(iStr);
  display.setFont(NULL); display.setCursor(78, 22); display.print("SYDNEY");
  display.setFont(&FreeSansBold9pt7b); char sStr[10]; sprintf(sStr, "%02d:%02d", s_h, s_m); display.setCursor(69, 46); display.print(sStr);
  display.setFont(NULL); display.setCursor(28, 56); display.print("Btn: next page");
}

void drawTimerPage() {
  drawHeader(timerRunning ? "STUDY TIMER" : (isTimerAlarm ? "TIME'S UP!" : "SET TIMER"));
  int mins = timerRemainingSec / 60, secs = timerRemainingSec % 60;
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", mins, secs);
  display.setFont(NULL); display.setTextColor(OLED_WHITE);
  if (isTimerAlarm) {
    display.setCursor(2, 24); display.print("Press ANY button");
    display.setCursor(2, 34); display.print("to stop alarm");
  }
  else if (!timerRunning) {
    display.setCursor(2, 20); display.print("Set Duration:");
    display.setCursor(78, 20); display.print("Tap=Exit");
    display.setFont(&FreeSansBold9pt7b);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
    int cx = (SCREEN_WIDTH - (int)w) / 2;
    display.setCursor(cx, 44); display.print(timeBuf);
    display.setFont(NULL);
    display.setCursor(2, 58); display.print("Tap:+5m Hold:+10m Dbl:Go");
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
    display.drawRect(14, 56, 100, 6, OLED_WHITE);
    if (timerDurationSec > 0) {
      int fill = (int)((float)(timerDurationSec - timerRemainingSec) / timerDurationSec * 96);
      if (fill < 0) fill = 0;
      display.fillRect(16, 57, fill, 4, OLED_WHITE);
    }
    display.setCursor(2, 58); display.print("Hold=Cancel Tap=Exit");
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

