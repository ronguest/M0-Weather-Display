//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <WiFi101.h>
#include <WiFiUDP.h>
#include <SD.h>
#include <Adafruit_STMPE610.h>
#include <JsonListener.h>
#include "credentials.h"
#include "WeatherClient.h"

// Red LED output on the M0 Feather
const int ledPin = 13;

// Configure TFT pins - specific to M0
#define STMPE_CS 6
#define TFT_CS   9
#define TFT_DC   10
#define SD_CS    5

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750

#define PENRADIUS 3

const int UPDATE_INTERVAL_SECS = 10 * 60;  // Update Conditions every 10 minutes, others update once/hour
//boolean USE_TOUCHSCREEN_WAKE = true;       // use the touchscreen to wake up, ~90mA current draw
//boolean DEEP_SLEEP = false;                 // use the touchscreen for deep sleep, ~10mA current draw but doesnt work
//int     AWAKE_TIME = 5;                   // how many seconds to stay 'awake' before going back to zzz

// TimeClient settings
const float UTC_OFFSET = -5;
int currentHour;
int currentDay;
int currentMinute;

boolean showForecastText = false;
