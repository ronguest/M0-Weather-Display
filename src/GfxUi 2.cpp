/**The MIT License (MIT)
Copyright (c) 2015 by Daniel Eichhorn
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
See more at http://blog.squix.ch
*/

#include "GfxUi.h"
//Adafruit_HX8357      tft    = Adafruit_HX8357(9, 10);

#ifdef HX8357
GfxUi::GfxUi(Adafruit_HX8357 *tft) {
  _tft = tft;
}
#endif

#ifdef ILI9341
GfxUi::GfxUi(Adafruit_ILI9341 *tft) {
  _tft = tft;
}
#endif

void GfxUi::drawString(int x, int y, char *text) {
  int16_t x1, y1;
  uint16_t w, h;
  _tft->setTextWrap(false);
  _tft->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  switch (_alignment) {
    case LEFT:
      x1 = x;
      break;
    case CENTER:
      x1 = x - w / 2;
      break;
    case RIGHT:
      x1 = x - w;
      break;
  }
  if (_textColor != _backgroundColor) {
    _tft->fillRect(x1, y - h -1, w + 1, h + 1, _backgroundColor);
  }
  _tft->setCursor(x1, y);
  _tft->print(text);
}

void GfxUi::drawString(int x, int y, String text) {
  char buf[text.length()+2];
  text.toCharArray(buf, text.length() + 1);
  drawString(x, y, buf);
}

void GfxUi::setTextColor(uint16_t c) {
  setTextColor(c, c);
}

void GfxUi::setTextSize(uint16_t x) {
  _tft->setTextSize(x);
}

void GfxUi::setTextColor(uint16_t c, uint16_t bg) {
  _textColor = c;
  _backgroundColor = bg;
  _tft->setTextColor(_textColor, _backgroundColor);
}

void GfxUi::setTextAlignment(TextAlignment alignment) {
  _alignment = alignment;
}

void GfxUi::drawBmp(char * filename, uint16_t x, uint16_t y) {
  char chars[50];
  Serial.print("Draw BMP: "); Serial.println(filename);
  ImageReturnCode stat;
  //stat = reader.drawBMP("/moon/22.bmp", tft, 0, 0);
  //reader.printStatus(stat);
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t GfxUi::read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t GfxUi::read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
