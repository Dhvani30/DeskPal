#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "../../DeskPal/OledTextFit.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

struct Box { std::string name; int x, y, w, h; };

class FakeDisplay {
 public:
  void setFont(const GFXfont* font) { font_ = font; }
  void setTextSize(uint8_t size) { size_ = size ? size : 1; }
  void setTextColor(uint16_t) {}
  void setCursor(int x, int y) { cursorX_ = x; cursorY_ = y; }
  void element(const std::string& name) { element_ = name; }
  const std::vector<Box>& textBoxes() const { return textBoxes_; }
  unsigned clippedPixels() const { return clipped_; }
  unsigned edgePixels() const { return edge_; }
  void clear() { pixels_.fill('.'); textBoxes_.clear(); clipped_ = edge_ = 0; }

  void getTextBounds(const char* text, int16_t x, int16_t y, int16_t* x1,
                     int16_t* y1, uint16_t* w, uint16_t* h) {
    int cx = x, cy = y;
    int minX = std::numeric_limits<int>::max(), minY = minX;
    int maxX = std::numeric_limits<int>::min(), maxY = maxX;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
      if (!font_) {
        minX = std::min(minX, cx); minY = std::min(minY, cy);
        maxX = std::max(maxX, cx + 6 * size_ - 1);
        maxY = std::max(maxY, cy + 8 * size_ - 1);
        cx += 6 * size_;
        continue;
      }
      if (*p < font_->first || *p > font_->last) continue;
      const GFXglyph& g = font_->glyph[*p - font_->first];
      if (g.width && g.height) {
        const int gx = cx + g.xOffset * size_, gy = cy + g.yOffset * size_;
        minX = std::min(minX, gx); minY = std::min(minY, gy);
        maxX = std::max(maxX, gx + g.width * size_ - 1);
        maxY = std::max(maxY, gy + g.height * size_ - 1);
      }
      cx += g.xAdvance * size_;
    }
    *x1 = maxX >= minX ? minX : x;
    *y1 = maxY >= minY ? minY : y;
    *w = maxX >= minX ? maxX - minX + 1 : 0;
    *h = maxY >= minY ? maxY - minY + 1 : 0;
  }

  void print(const char* text) {
    int16_t x1, y1; uint16_t w, h;
    getTextBounds(text, cursorX_, cursorY_, &x1, &y1, &w, &h);
    if (!w || !h) return;
    textBoxes_.push_back({element_ + ": " + text, x1, y1, w, h});
    // Filling the measured glyph bounding box is conservative: any ink from
    // the real Adafruit font must fit inside this same getTextBounds box.
    fillRect(x1, y1, w, h, '*');
  }

  void fillRect(int x, int y, int w, int h, char value = '#') {
    for (int py = y; py < y + h; ++py)
      for (int px = x; px < x + w; ++px) pixel(px, py, value);
  }
  void drawRect(int x, int y, int w, int h, char value = '#') {
    drawLine(x, y, x + w - 1, y, value); drawLine(x, y + h - 1, x + w - 1, y + h - 1, value);
    drawLine(x, y, x, y + h - 1, value); drawLine(x + w - 1, y, x + w - 1, y + h - 1, value);
  }
  void drawLine(int x0, int y0, int x1, int y1, char value = '#') {
    const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
      pixel(x0, y0, value);
      if (x0 == x1 && y0 == y1) break;
      const int twice = 2 * error;
      if (twice >= dy) { error += dy; x0 += sx; }
      if (twice <= dx) { error += dx; y0 += sy; }
    }
  }
  void fillCircle(int cx, int cy, int r, char value = '#') {
    for (int y = -r; y <= r; ++y)
      for (int x = -r; x <= r; ++x)
        if (x * x + y * y <= r * r) pixel(cx + x, cy + y, value);
  }
  void drawBitmap(int x, int y, const unsigned char*, int w, int h, char value = '#') {
    fillRect(x, y, w, h, value);
  }
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, char value = '#') {
    const int left = std::min({x0, x1, x2}), right = std::max({x0, x1, x2});
    const int top = std::min({y0, y1, y2}), bottom = std::max({y0, y1, y2});
    for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) pixel(x, y, value);
  }
  void dump(std::ostream& out) const {
    for (int y = 0; y < 64; ++y) { out.write(&pixels_[y * 128], 128); out << '\n'; }
  }

 private:
  void pixel(int x, int y, char value) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) { ++clipped_; return; }
    if (x == 0 || x == 127 || y == 63) ++edge_;
    pixels_[y * 128 + x] = value;
  }
  const GFXfont* font_ = nullptr;
  uint8_t size_ = 1;
  int cursorX_ = 0, cursorY_ = 0;
  std::string element_ = "element";
  std::array<char, 128 * 64> pixels_{};
  std::vector<Box> textBoxes_;
  unsigned clipped_ = 0, edge_ = 0;
};

static unsigned failures = 0;

void require(bool ok, const std::string& page, const std::string& why) {
  if (!ok) { std::cerr << "FAIL " << page << ": " << why << '\n'; ++failures; }
}

void fit(FakeDisplay& d, const std::string& page, const char* label, const char* text,
         int x, int y, const GFXfont* font = nullptr, int width = 126,
         OledTextAlign align = OLED_TEXT_LEFT) {
  d.element(label);
  drawOledTextFit(d, text, x, y, font, width, align);
  const Box& b = d.textBoxes().back();
  require(b.x >= 1 && b.x + b.w - 1 <= 126 && b.y >= 0 && b.y + b.h - 1 <= 62,
          page, b.name + " bounds x=" + std::to_string(b.x) + " y=" + std::to_string(b.y) +
          " w=" + std::to_string(b.w) + " h=" + std::to_string(b.h));
}

void header(FakeDisplay& d, const std::string& page, const char* title) {
  d.fillRect(1, 0, 126, 16);
  fit(d, page, "header", title, 64, 4, nullptr, 126, OLED_TEXT_CENTER);
}

void checkPage(FakeDisplay& d, const std::string& page) {
  const auto& boxes = d.textBoxes();
  for (size_t i = 0; i < boxes.size(); ++i) for (size_t j = i + 1; j < boxes.size(); ++j) {
    const Box& a = boxes[i]; const Box& b = boxes[j];
    const bool overlap = a.x < b.x + b.w && b.x < a.x + a.w &&
                         a.y < b.y + b.h && b.y < a.y + a.h;
    require(!overlap, page, "overlapping text elements: " + a.name + " and " + b.name);
  }
  require(d.clippedPixels() == 0, page, "clipped pixel count=" + std::to_string(d.clippedPixels()));
  require(d.edgePixels() == 0, page, "edge pixel count=" + std::to_string(d.edgePixels()));
}

void writeDump(const std::string& page, const std::string& frame, const FakeDisplay& d) {
  static const std::filesystem::path dir = "tools/oled_dump";
  std::filesystem::create_directories(dir);
  std::ofstream out(dir / (page + ".txt"), frame == "normal" ? std::ios::trunc : std::ios::app);
  out << "=== " << frame << " ===\n";
  d.dump(out);
}

void renderEyes(const std::string& frame) {
  const std::string page = "eyes"; FakeDisplay d; d.clear();
  d.fillRect(18, 14, 36, 36); d.fillRect(74, 14, 36, 36);
  d.fillRect(27, 23, 18, 18); d.fillRect(83, 23, 18, 18); // pupils inside eye whites
  fit(d, page, "quiet status", "M", 120, 0, nullptr, 6);
  checkPage(d, page); writeDump(page, frame, d);
}

void renderClock(const std::string& frame) {
  const std::string page = "clock"; FakeDisplay d; d.clear();
  header(d, page, frame == "worst" ? "CLOCK (DEMO)" : "CLOCK");
  fit(d, page, "time", "23:59", 64, 19, &FreeSansBold18pt7b, 126, OLED_TEXT_CENTER);
  fit(d, page, "date", "Wed, Dec 31", 64, 51, nullptr, 126, OLED_TEXT_CENTER);
  d.drawRect(94, 3, 18, 9); d.fillRect(112, 5, 2, 5); // battery
  d.drawLine(118, 8, 124, 4); // wifi mark
  checkPage(d, page); writeDump(page, frame, d);
}

void renderWeather(const std::string& frame) {
  const std::string page = "weather"; FakeDisplay d; d.clear(); header(d, page, "WEATHER");
  if (frame == "normal") {
    fit(d, page, "city", "INTERNATIONALCITY", 1, 22, nullptr, 96);
    d.drawBitmap(109, 19, nullptr, 16, 16);
    fit(d, page, "outside", "Outside: -10.5 C", 1, 34);
    fit(d, page, "feels", "Feels: -10.5 C RH:100%", 1, 45);
    fit(d, page, "description", "Thunderstorms with heavy rain and strong winds", 1, 55);
  } else {
    fit(d, page, "wifi error", "WiFi not connected", 1, 27);
    fit(d, page, "setup hint", "Setup: hold touch at boot", 1, 39);
  }
  checkPage(d, page); writeDump(page, frame, d);
}

void renderForecast(const std::string& frame) {
  const std::string page = "forecast"; FakeDisplay d; d.clear(); header(d, page, "NEXT 3 DAYS");
  if (frame == "normal") {
    const int left[] = {1, 44, 87}, width[] = {40, 40, 40};
    const char* days[] = {"Wed", "Thu", "Fri"}; const char* temps[] = {"-100 C", "100 C", "-10 C"};
    d.drawLine(42, 18, 42, 60); d.drawLine(85, 18, 85, 60);
    for (int i = 0; i < 3; ++i) {
      const int center = left[i] + width[i] / 2;
      fit(d, page, "day", days[i], center, 19, nullptr, width[i], OLED_TEXT_CENTER);
      d.drawBitmap(center - 8, 30, nullptr, 16, 16);
      fit(d, page, "temperature", temps[i], center, 50, nullptr, width[i], OLED_TEXT_CENTER);
    }
  } else {
    fit(d, page, "forecast error", "Forecast HTTP 503", 1, 28);
    fit(d, page, "weather hint", "Check WEATHER page", 1, 42);
  }
  checkPage(d, page); writeDump(page, frame, d);
}

void renderRoom(const std::string& frame) {
  const std::string page = "room"; FakeDisplay d; d.clear(); header(d, page, "ROOM");
  if (frame == "normal") {
    fit(d, page, "temperature", "-10.5 C", 1, 24, nullptr, 84);
    fit(d, page, "humidity", "Humidity  100%", 1, 41, nullptr, 108);
    for (int x = 64; x < 124; ++x) d.drawLine(x, 50 + (x % 6), x + 1, 50 + ((x + 1) % 6));
  } else {
    fit(d, page, "sensor error", "Sensor recovering...", 1, 29);
    fit(d, page, "sensor hint", "Check BME280 wiring", 1, 42);
  }
  checkPage(d, page); writeDump(page, frame, d);
}

void renderInsights(const std::string& frame) {
  const std::string page = "insights"; FakeDisplay d; d.clear(); header(d, page, "INSIGHTS");
  if (frame == "normal") {
    fit(d, page, "title", "ROOM COMFORTABLE", 1, 19);
    fit(d, page, "score", "Comfort estimate: 100", 1, 29);
    fit(d, page, "dew label", "Dew point estimate", 1, 38);
    fit(d, page, "dew point", "-10.5 C", 1, 47);
    fit(d, page, "action", "Ventilate or cool the room", 1, 55);
  } else {
    fit(d, page, "title", "Default temp range", 1, 21);
    fit(d, page, "range", "Your range: -10-38C", 1, 34);
    fit(d, page, "votes", "Votes: 4294967295 (learning)", 1, 46);
    fit(d, page, "hint", "Rate it on dashboard", 1, 57);
  }
  checkPage(d, page); writeDump(page, frame, d);
  if (frame == "worst") {
    FakeDisplay forecast; forecast.clear(); header(forecast, page, "INSIGHTS");
    fit(forecast, page, "forecast title", "Forecast estimate", 1, 22);
    fit(forecast, page, "pressure", "Storm risk", 1, 34);
    fit(forecast, page, "temperature estimate", "Hot estimate ~999 min", 1, 48);
    checkPage(forecast, page); writeDump(page, "forecast-panel", forecast);
  }
}

void renderTimer(const std::string& frame) {
  const std::string page = "timer"; FakeDisplay d; d.clear(); header(d, page, "TIMER");
  fit(d, page, "state", frame == "worst" ? "TIME UP" : "RUNNING", 64, 21, nullptr, 126, OLED_TEXT_CENTER);
  fit(d, page, "countdown", "120:00", 64, 33, &FreeSansBold18pt7b, 126, OLED_TEXT_CENTER);
  if (frame == "worst") {
    d.fillRect(60, 2, 67, 12); d.drawRect(60, 2, 67, 12);
    fit(d, page, "timer overlay", "RUN 120:00", 63, 4, nullptr, 61);
  }
  checkPage(d, page); writeDump(page, frame, d);
}

void renderConfig(const std::string& frame) {
  const std::string page = "config"; FakeDisplay d; d.clear();
  fit(d, page, "portal title", "Config mode", 1, 0);
  fit(d, page, "connect", "Connect to:", 1, 8);
  fit(d, page, "ap name", frame == "worst" ? "DeskPal-FFFFFF" : "DeskPal-1A2B3C", 1, 16);
  fit(d, page, "pin label", "PIN:", 1, 24);
  fit(d, page, "pin", "99999999", 1, 32);
  fit(d, page, "url", "Open 192.168.4.1", 1, 40);
  checkPage(d, page); writeDump(page, frame, d);
}

void renderSplash(const std::string& frame) {
  const std::string page = "splash"; FakeDisplay d; d.clear();
  for (int step = 0; step < 20; ++step) {
    int w = 2 + step * 6, h = 2 + step * 3;
    d.fillRect((128 - w) / 2, (64 - h) / 2, w, h);
  }
  d.clear(); fit(d, page, "boot title", "DeskPal", 64, 20, &FreeSansBold9pt7b, 126, OLED_TEXT_CENTER);
  checkPage(d, page); writeDump(page, frame, d);
}

int main() {
  for (const std::string frame : {"normal", "worst"}) {
    renderEyes(frame); renderClock(frame); renderWeather(frame); renderForecast(frame);
    renderRoom(frame); renderInsights(frame); renderTimer(frame); renderConfig(frame); renderSplash(frame);
  }
  if (failures) { std::cerr << failures << " OLED bounds checks failed\n"; return 1; }
  std::cout << "PASS eyes, clock, weather, forecast, room, insights, timer, config portal, boot splash\n";
  std::cout << "PASS worst cases: long text, negative/3-digit values, max timer, Wi-Fi/error states\n";
  std::cout << "PASS clipped pixels: 0; edge pixels: 0; overlapping text boxes: 0\n";
  std::cout << "ASCII dumps: tools/oled_dump/*.txt\n";
  return 0;
}
