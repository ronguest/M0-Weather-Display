//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#pragma once

// ***** Only one of these should be set - HX8357 is the large TFT display, ILI9341 is the original
//#define ILI9341 1
#define HX8357 1

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#ifdef HX8357
#include <Adafruit_HX8357.h>
#endif
#ifdef ILI9341
#include <Adafruit_ILI9341.h>
#endif

#ifdef HX8357
#define WX_BLACK HX8357_BLACK
#define WX_CYAN HX8357_CYAN
#define WX_BLUE HX8357_BLUE
#define WX_WHITE HX8357_WHITE
#endif
#ifdef ILI9341
#define WX_BLACK ILI9341_BLACK
#define WX_CYAN ILI9341_CYAN
#define WX_BLUE ILI9341_BLUE
#define WX_WHITE ILI9341_WHITE
#endif

#ifdef HX8357
#include "Fonts/FreeSansBold9pt7b.h"    // Font from Adafruit Gfx library
#include "ArialRoundedMTBold_36.h"    // Font created by http://oleddisplay.squix.ch/
#define smallFont FreeSansBold9pt7b
#define largeFont ArialRoundedMTBold_36
const int maxPerLine = 34;    // Max chars that fit on line for text forecast info
const int lineSize = 22;            // Vertical line space increment for forecast text
#endif
#ifdef ILI9341
#include "ArialRoundedMTBold_14.h"    // Font created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_36.h"    // Font created by http://oleddisplay.squix.ch/
#define smallFont ArialRoundedMTBold_14
#define largeFont ArialRoundedMTBold_36
const int maxPerLine = 38;     // Max chars that fit on line for text forecast info
const int lineSize = 20;            // Vertical line space increment for forecast text
#endif
