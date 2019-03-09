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
#include <TimeLib.h>
#include <Timezone.h>
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

int status = WL_IDLE_STATUS;

// NTP Servers:
//static const char ntpServerName[] = "us.pool.ntp.org";
static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";
const int timeZone = 0;     // Using the Timezone library now which does it's own TZ and DST correction
boolean ntpSuccess = false;   // To enable logging failure without flooding failures

//US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, dowSunday, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, dowSunday, Nov, 2, -360};
Timezone usCT(usCDT, usCST);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get the TZ abbrev
time_t local;
unsigned int hours = 0;                      // Track hours
unsigned int minutes = 0;                    // Track minutes
unsigned int seconds = 0;                    // Track seconds
unsigned int dayOfWeek = 0;                  // Sunday == 1

unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

////// Code and variables for NTP syncing
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

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
