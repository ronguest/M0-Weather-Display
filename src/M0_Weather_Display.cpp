//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#include "display.h"
#include "settings.h"

#ifdef HX8357
#define TFT_RST -1
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
#endif
#ifdef ILI9341
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
#endif
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

WeatherClient weather(true);

//declaring prototypes
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
typedef void (*ProgressCallback)(String fileName, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void showOverview();
void showForecastDetail();
int drawForecastText(int y, String text, int maxlines);
void drawCurrentWeather();
void drawForecastDetail(uint16_t x, uint16_t y, String day, String low, String high, String icon);
void drawForecast();
void drawAstronomy();
void drawTime();
void sleepNow(int wakeup);
void todayDetail(int baseline);

long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw

ImageReturnCode stat; // Status from image-reading functions
Adafruit_ImageReader reader;     // Class w/image-reading functions

void drawString(int x, int y, String s) {
  tft.setCursor(x, y);
  tft.println(s);
}
void drawBmp(String filename, int x, int y) {
  // Notice the 'reader' object performs this, with 'tft' as an argument.
  Serial.print(F("Loading moon.bmp to screen..."));
  stat = reader.drawBMP("/moon/22.bmp", tft, 0, 0);
  reader.printStatus(stat);   // How'd we do?  
}
void setup(void) {
  time_t ntpTime;
  Serial.begin(115200);
  delay(5000);

  Serial.println("Starting up");

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  weather.init();

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(WX_BLACK);
  tft.setFont(&smallFont);
  tft.setTextColor(WX_WHITE, WX_BLACK);
  //tft.setTextAlignment(CENTER);

  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");

  // Set up SD card to read icons/moon files
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD failed!");
  }
  Serial.println("SD OK!");

  delay(5000);
  tft.fillScreen(WX_BLUE);
  Serial.println("Stopping in Setup");
  while (1) delay(1000);

  currentHour = -1;     // Causes an immediate update to weather data on startup
}

void loop() {
  // Check if we should update weather information
  if ((millis() - lastDownloadUpdate) > (1000 * UPDATE_INTERVAL_SECS)) {
    Serial.println("In update code");
    // Always display overview after an update
    showForecastText = false;
    updateData();                   // This calls showOverview when done
    drawString(10,10,"Updating...");
    lastDownloadUpdate = millis();
  }
  // If user touches screen, toggle between weather overview and the detailed forecast text
  if (ts.touched()) {
    Serial.println("Screen touched");
    drawString(10,10,"Touhed...");
    showForecastText = !showForecastText;
    if (showForecastText) {
      showForecastDetail();
    } else {
      showOverview();
    }
  }
  delay(100);
}

// Download latest weather data and update screen
void updateData() {
  time_t local;
  time_t ntpTime;
  int thisHour;

  //local = usCT.toLocal(now(), &tcr);
  //thisHour = hour(local);

  weather.updateConditions(AW_DEVICE, AW_APP_KEY, AW_API_KEY);
  // We only update the Forecast once an hour. They don't change much
  if (thisHour != currentHour) {
    currentHour = thisHour;
    weather.updateForecast(WUNDERGROUND_POSTAL_KEY, WUNDERGRROUND_API_KEY);
  }

  showOverview();
}

// Makes calls to paint the first screen
void showOverview() {
  tft.fillScreen(WX_BLACK);

  drawTime();
  drawCurrentWeather();
  todayDetail(130);       // Parameter is y coordinate to start forecast text
  drawForecast();
  drawAstronomy();
}

// On first page draw today's forecast
void todayDetail(int baseline) {
  String text;
  int maxLines = 5;

  //local = usCT.toLocal(now(), &tcr);
  //hours = hour(local);

  text = weather.getTodayForecastTextAM();
  // Recently WU starting sending "null" for today's AM forecast text starting at 3pm local
  if (text == "null") {
    text = weather.getTodayForecastTextPM();
  }

  drawForecastText(baseline, text, maxLines);
}

int drawForecastText(int y, String text, int maxLines) {
  int textLength;
  int finalSpace;
  int startPoint = 0;   // Position in text of next character to print

  tft.setFont(&smallFont);
  tft.setTextColor(WX_CYAN, WX_BLACK);
  //ui.setTextAlignment(LEFT);
  textLength = text.length();

  while ((startPoint < textLength) && (maxLines > 0)) {
    // Find the last space in the next string we will print
    finalSpace = text.lastIndexOf(' ', startPoint + maxPerLine);
    if (finalSpace == -1 ) {
      // It's possible the final substring doesn't have a space
      finalSpace = textLength;
    }
    //Serial.print("Final space: ");Serial.println(finalSpace);
    // If the first character is a space, skip it (happens due to line wrapping)
    if (text.indexOf(' ', startPoint) == startPoint) {
      startPoint++;
    }
    drawString(10, y, text.substring(startPoint, finalSpace));
    y += lineSize;
    startPoint = finalSpace;
    //Serial.print("Start point: ");Serial.println(startPoint);
    maxLines--;
  }
  return y;
}

// On second page, draw today's AM, PM and tomorrow's AM, PM forecast text
void showForecastDetail() {
  int textLength;
  String text;
  int maxLines = 7;
  int y = 30;
  int period;

  tft.fillScreen(WX_BLACK);
  tft.setFont(&smallFont);
  tft.setTextColor(WX_CYAN, WX_BLACK);
  //ui.setTextAlignment(LEFT);

  // We show up to 4 periods which is 2 days + 2 nights
  for (period=0; period<4; period++) {
    //Serial.print("period: ");Serial.println(period);
    switch (period) {
      case 0:
        text = weather.getTodayForecastTextAM();
        if (text == "null") {
          // Skip today if null
          continue;
        }
        break;
      case 1:
        text = weather.getTodayForecastTextPM();
        break;
      case 2:
        text = weather.getTomorrowForecastTextAM();
        break;
      case 3:
        text = weather.getTomorrowForecastTextPM();
        break;
      default:
        return;
    }
    //Serial.print("forecast text: ");Serial.println(text);
    textLength = text.length();
    //Serial.print("Forecast length: "); Serial.println(textLength);

    // Draw the name of the period (e.g. "Monday" or "Monday Night")
    String title = (period < 2) ? weather.getTodayName() : weather.getTomorrowName();
    if ((period == 1 || period == 3)) {
      title = title + " " + "night";
    }
    drawString(0, y, title);
    y += lineSize;
    y = drawForecastText(y, text, maxLines);
    y += lineSize;    // Add extra space between forecasts
  }
}

// Draws date and time at top of display
void drawTime() {
/*  local = usCT.toLocal(now(), &tcr);
  hours = hour(local);
  minutes = minute(local);
  dayOfWeek = weekday(local);
*/
  //ui.setTextAlignment(CENTER);
  tft.setTextColor(WX_WHITE, WX_BLACK);
  tft.setFont(&smallFont);

  /*String dateS = String(dayStr(dayOfWeek)) + ", " + String(monthStr(month(local))) + " " + String(day(local));
  drawString(150, 20, dateS);

  String ampm = "am";
  if (hours >= 12) {
    hours = (hours > 12) ? hours - 12 : hours;
    ampm = "pm";
  }
  String timeS = String(hours) + ":" + (minutes < 10 ?"0" : "" ) + String(minutes) + ampm;
  drawString(150, 40, timeS);*/
}

// draws current weather information -- which is just the current temperature
void drawCurrentWeather() {
  tft.setFont(&largeFont);
  tft.setTextColor(WX_CYAN, WX_BLACK);
  //ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  float f = weather.getCurrentTemp().toFloat();
  f = f + 0.5f;
  int temp = (int) f;
  drawString(180, 100, String(f).substring(0,2) + degreeSign);
}

// On first page draw hi, lo and icon for today and tomorrow
void drawForecast() {
  const int drop = 260;
  String icon;

  // Handle WU returning null starting at 3pm
  icon = weather.getTodayIcon();
  if (icon == "null") {
    icon = weather.getTonightIcon();
  }
  drawForecastDetail(30, drop, weather.getTodayName(), weather.getTodayForecastLow(), weather.getTodayForecastHigh(), icon);

  drawForecastDetail(200, drop, weather.getTomorrowName(), weather.getTomorrowForecastLow(), weather.getTomorrowForecastHigh(), weather.getTomorrowIcon());
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, String day, String low, String high, String icon) {
  tft.setTextColor(WX_CYAN, WX_BLACK);
  tft.setFont(&smallFont);
  //ui.setTextAlignment(CENTER);
  day = day.substring(0, 3);
  day.toUpperCase();
  drawString(x + 45, y, day);

  tft.setFont(&largeFont);
  tft.setTextColor(WX_WHITE, WX_BLACK);
  if (high == "null") {
    // Omit daytime high if null from WU
    drawString(x + 40, y + 40, low);
  } else {
    drawString(x + 40, y + 40, low + "|" + high);
  }

  drawBmp("/Icons/" + icon + ".bmp", x+0, y + 40);
}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  int baseline = 410;   // Place at the bottom
  int baseX = 20;
  String moonFile;

  moonFile = "/Moon/" + weather.getMoonAge() + ".bmp";
  Serial.println("Load moon file: " + moonFile);
  drawBmp(moonFile, 140, baseline+5);

  tft.setTextColor(WX_WHITE, WX_BLACK);
  tft.setFont(&smallFont);
  //ui.setTextAlignment(LEFT);
  tft.setTextColor(WX_CYAN, WX_BLACK);
  drawString(baseX, baseline+25, "Sun");
  tft.setTextColor(WX_WHITE, WX_BLACK);
  drawString(baseX, baseline+43, weather.getSunriseTime());
  drawString(baseX, baseline+60, weather.getSunsetTime());

  //ui.setTextAlignment(RIGHT);
  tft.setTextColor(WX_CYAN, WX_BLACK);
  drawString(baseX+280, baseline+25, "Moon");
  tft.setTextColor(WX_WHITE, WX_BLACK);
  drawString(baseX+280, baseline+43, weather.getMoonriseTime());
  drawString(baseX+280, baseline+60, weather.getMoonsetTime());

}

