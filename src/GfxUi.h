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

#include "display.h"
//#include <FS.h>
#include <SD.h>
#ifdef HX8357
#include <Adafruit_HX8357.h>
#endif
#ifdef ILI9341
#include <Adafruit_ILI9341.h>
#endif

#ifndef _GFX_UI_H
#define _GFX_UI_H


#define BUFFPIXEL 20

enum TextAlignment {
  LEFT, CENTER, RIGHT
};

class GfxUi {
  public:
#ifdef HX8357
    GfxUi(Adafruit_HX8357 * tft);
#endif
#ifdef ILI9341
    GfxUi(Adafruit_ILI9341 * tft);
#endif
    void drawString(int x, int y, char *text);
    void drawString(int x, int y, String text);
    void setTextAlignment(TextAlignment alignment);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t bg);
    void setTextSize(uint16_t x);
    void drawBmp(String filename, uint16_t x, uint16_t y);

  private:
  #ifdef HX8357
  Adafruit_HX8357 * _tft;
  #endif
  #ifdef ILI9341
    Adafruit_ILI9341 * _tft;
  #endif
    TextAlignment _alignment = LEFT;
    uint16_t _textColor;
    uint16_t _backgroundColor;
    uint16_t read16(File &f);
    uint32_t read32(File &f);
    int freeRam();
};

#endif
