//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#pragma once

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_HX8357.h>

#define WX_BLACK HX8357_BLACK
#define WX_CYAN HX8357_CYAN
#define WX_BLUE HX8357_BLUE
#define WX_WHITE HX8357_WHITE

#include "Fonts/FreeSansBold9pt7b.h"    // Font from Adafruit Gfx library
#include "ArialRoundedMTBold_36.h"    // Font created by http://oleddisplay.squix.ch/
#define smallFont FreeSansBold9pt7b
#define largeFont ArialRoundedMTBold_36
const int maxPerLine = 34;    // Max chars that fit on line for text forecast info
const int lineSize = 22;            // Vertical line space increment for forecast text
