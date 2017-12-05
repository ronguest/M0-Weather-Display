//
//  Copyright (C) 2017 Ronald Guest <http://about.me/ronguest>

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <Adafruit_HX8357.h>
#include <SD.h>
#include <Adafruit_STMPE610.h>
#include <JsonListener.h>
#include <Adafruit_WINC1500.h>
#include <Adafruit_WINC1500Udp.h>
#include <TimeLib.h>
#include <Timezone.h>
#include "credentials.h"
#include "WundergroundClient.h"

#ifdef HX8357
#include "Fonts/FreeSansBold18pt7b.h"    // Font from Adafruit Gfx library
#include "ArialRoundedMTBold_36.h"    // Font created by http://oleddisplay.squix.ch/
#define smallFont FreeSansBold18pt7b
#define largeFont ArialRoundedMTBold_36
#endif
#ifdef ILI9341
#include "ArialRoundedMTBold_14.h"    // Font created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_36.h"    // Font created by http://oleddisplay.squix.ch/
#define smallFont ArialRoundedMTBold_14
#define largeFont ArialRoundedMTBold_36
#endif

// Additional UI functions
#include "GfxUi.h"

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

// Define the WINC1500 board connections below.
// If you're following the Adafruit WINC1500 board
// guide you don't need to modify these:
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2     // or, tie EN to VCC and comment this out
// The SPI pins of the WINC1500 (SCK, MOSI, MISO) should be
// connected to the hardware SPI port of the Arduino.
// On an Uno or compatible these are SCK = #13, MISO = #12, MOSI = #11.
// On an Arduino Zero use the 6-pin ICSP header, see:
//   https://www.arduino.cc/en/Reference/SPI

// Setup the WINC1500 connection with the pins above and the default hardware SPI.
Adafruit_WINC1500 WiFi(WINC_CS, WINC_IRQ, WINC_RST);

// Or just use hardware SPI (SCK/MOSI/MISO) and defaults, SS -> #10, INT -> #7, RST -> #5, EN -> 3-5V
//Adafruit_WINC1500 WiFi;

//Adafruit_WINC1500Client client;

int keyIndex = 0;                // your network key Index number (needed only for WEP)
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

Adafruit_WINC1500UDP Udp;
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

// Wunderground Settings
const boolean IS_METRIC = false;
const String WUNDERGRROUND_LANGUAGE = "EN";

boolean showForecastText = false;
