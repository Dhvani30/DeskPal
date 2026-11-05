#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <Adafruit_GFX.h>

enum OledTextAlign { OLED_TEXT_LEFT, OLED_TEXT_CENTER, OLED_TEXT_RIGHT };

// Measure first, shorten only at a UTF-8 boundary, then place the measured
// glyph box in the safe area. y is the desired glyph-box top, not baseline.
template <typename Display>
bool drawOledTextFit(Display& display, const char* text, int x, int y,
                     const GFXfont* font, int maxWidth,
                     OledTextAlign align = OLED_TEXT_LEFT) {
  if (!text) return false;
  display.setFont(font);
  display.setTextSize(1);
  maxWidth = maxWidth < 1 ? 1 : (maxWidth > 126 ? 126 : maxWidth);

  char fitted[96];
  size_t length = 0;
  while (length < sizeof(fitted) - 1 && text[length]) ++length;
  memcpy(fitted, text, length);
  fitted[length] = '\0';

  int16_t bx = 0, by = 0;
  uint16_t bw = 0, bh = 0;
  display.getTextBounds(fitted, 0, 0, &bx, &by, &bw, &bh);
  if (bw > maxWidth) {
    static const char ellipsis[] = "...";
    bool found = false;
    size_t prefix = length;
    while (prefix > 0) {
      --prefix;
      while (prefix > 0 && (((uint8_t)text[prefix] & 0xC0U) == 0x80U))
        --prefix;
      const size_t dots = sizeof(ellipsis) - 1;
      const size_t safePrefix = prefix < sizeof(fitted) - dots - 1
          ? prefix : sizeof(fitted) - dots - 1;
      memcpy(fitted, text, safePrefix);
      memcpy(fitted + safePrefix, ellipsis, dots + 1);
      display.getTextBounds(fitted, 0, 0, &bx, &by, &bw, &bh);
      if (bw <= maxWidth) { found = true; break; }
    }
    if (!found) {
      fitted[0] = '\0';
      display.getTextBounds(fitted, 0, 0, &bx, &by, &bw, &bh);
    }
  }

  if (bw == 0 || bh == 0) return true;
  const int boxWidth = (int)bw;
  int left = align == OLED_TEXT_CENTER ? x - boxWidth / 2 :
             align == OLED_TEXT_RIGHT ? x - boxWidth + 1 : x;
  if (left < 1) left = 1;
  if (left + boxWidth - 1 > 126) left = 127 - boxWidth;
  if (left < 1) left = 1;

  int top = y;
  if (top < 0) top = 0;
  if (top + (int)bh - 1 > 62) top = 63 - (int)bh;
  if (top < 0) top = 0;

  // Adafruit_GFX custom fonts use baseline coordinates; the built-in font
  // uses top-left coordinates. getTextBounds supplies the correct offsets.
  const int cursorX = left - bx;
  const int cursorY = top - by;
  display.setCursor(cursorX, cursorY);
  display.print(fitted);
  return true;
}
