#pragma once

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

