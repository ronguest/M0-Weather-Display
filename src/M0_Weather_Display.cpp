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

GfxUi ui = GfxUi(&tft);
SdFat                SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys
ImageReturnCode imageStatus; 	// Status from image-reading functions

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
time_t getNtpTime();
void sendNTPpacket(IPAddress&);
void todayDetail(int baseline);

long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw
boolean updateSuccess;

void setup(void) {
  time_t ntpTime;
  Serial.begin(115200);
  delay(2000);

  //Configure pins for Adafruit M0 ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.println("FeatherWing TFT");
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(WX_BLACK);
  tft.setFont(&smallFont);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(CENTER);
  ui.drawString(120, 160, F("Connecting to WiFi"));

  yield();

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("No Wifi"));
    // don't continue
    while (true) delay(1000);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print(F("Wifi connect to: ")); Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection
    delay(5000);
  }

  // Set up SD card to read icons/moon files
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS, SD_SCK_MHZ(25))) {
    Serial.println("SD failed!");
  }
  Serial.println("SD OK!");

  // Get current time using NTP
  Udp.begin(localPort);
  ntpTime = getNtpTime();
  if (ntpTime != 0) {
    setTime(ntpTime);
  } else {
    Serial.println("Failed to set the initial time");
  }

  currentHour = -1;     // Causes an immediate update to weather data on startup
}

void loop() {
  // Check if we should update weather information
  if ((millis() - lastDownloadUpdate) > (1000L * UPDATE_INTERVAL_SECS)) {
    Serial.println("millis " + String(millis()));
    Serial.println("lastDownload " + String(lastDownloadUpdate));
    // Always display overview after an update
    showForecastText = false;
    updateData();
    lastDownloadUpdate = millis();
    Serial.println("updated lastDownload " + String(lastDownloadUpdate));

	if (updateSuccess) {
		tft.fillCircle(circleX, circleY, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(circleX, circleY, 5, HX8357_RED);
	}		
  }
  // If user touches screen, toggle between weather overview and the detailed forecast text
  if (ts.touched()) {
    showForecastText = !showForecastText;
    if (showForecastText) {
      showForecastDetail();
    } else {
      showOverview();
    }
	if (updateSuccess) {
		tft.fillCircle(circleX, circleY, 5, HX8357_GREEN);
	} else {
		tft.fillCircle(circleX, circleY, 5, HX8357_RED);
	}	
  }
  delay(100);
}

// Download latest weather data and update screen
void updateData() {
	time_t local;
	time_t ntpTime;
	int thisHour;
	boolean success1 = true;
	boolean success2 = true;

	local = usCT.toLocal(now(), &tcr);
	thisHour = hour(local);

//	success1 = weather.updateConditions(AW_DEVICE, AW_APP_KEY, AW_API_KEY);
	success1 = weather.updateConditions(AW_DEVICE, AW_APP_KEY, WUNDERGRROUND_API_KEY);
	// We only update the Forecast once an hour. They don't change much
  //return;
	if (thisHour != currentHour) {
		currentHour = thisHour;
		success2 = weather.updateForecast(WUNDERGROUND_POSTAL_KEY, WUNDERGRROUND_API_KEY);
		// Try an NTP time sync so we don't get too far off
		ntpTime = getNtpTime();
		if (ntpTime != 0) {
			setTime(ntpTime);
		}
	}
	updateSuccess = success1 & success2;
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

  local = usCT.toLocal(now(), &tcr);
  hours = hour(local);

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
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(LEFT);
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
    ui.drawString(10, y, text.substring(startPoint, finalSpace));
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
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(LEFT);

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
    ui.drawString(0, y, title);
    y += lineSize;
    y = drawForecastText(y, text, maxLines);
    y += lineSize;    // Add extra space between forecasts
  }
}

// Draws date and time at top of display
void drawTime() {
  local = usCT.toLocal(now(), &tcr);
  hours = hour(local);
  minutes = minute(local);
  dayOfWeek = weekday(local);

  ui.setTextAlignment(CENTER);
  ui.setTextColor(WX_WHITE, WX_BLACK);
  tft.setFont(&smallFont);

  String dateS = String(dayStr(dayOfWeek)) + ", " + String(monthStr(month(local))) + " " + String(day(local));
  ui.drawString(150, 20, dateS);

  String ampm = "am";
  if (hours >= 12) {
    hours = (hours > 12) ? hours - 12 : hours;
    ampm = "pm";
  }
  String timeS = String(hours) + ":" + (minutes < 10 ?"0" : "" ) + String(minutes) + ampm;
  ui.drawString(150, 40, timeS);
}

// draws current weather information -- which is just the current temperature
void drawCurrentWeather() {
  tft.setFont(&largeFont);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  float f = weather.getCurrentTemp().toFloat();
  f = f + 0.5f;
  int temp = (int) f;
  ui.drawString(180, 100, temp + degreeSign);
  //ui.drawString(180, 100, String(f).substring(0,2) + degreeSign);
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
  ui.setTextColor(WX_CYAN, WX_BLACK);
  tft.setFont(&smallFont);
  ui.setTextAlignment(CENTER);
  day = day.substring(0, 3);
  day.toUpperCase();
  ui.drawString(x + 45, y, day);

  tft.setFont(&largeFont);
  ui.setTextColor(WX_WHITE, WX_BLACK);
  if (high == "null") {
    // Omit daytime high if null from WU
    ui.drawString(x + 40, y + 40, low);
  } else {
    ui.drawString(x + 40, y + 40, low + "|" + high);
  }

  char file[64] = "/Icons/";
  strcat(file, icon.c_str());
  strcat(file, ".bmp");
  Serial.print("Load icon file: "); Serial.println(file);
  imageStatus = reader.drawBMP(file, tft, x+0, y + 40);
  reader.printStatus(imageStatus);   // How'd we do?  
}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  int baseline = 410;   // Place at the bottom
  int baseX = 20;

  char file[64] = "/Moon/";
  strcat(file, weather.getMoonAge().c_str());
  strcat(file, ".bmp");
  Serial.print("Load moon file: "); Serial.println(file);
  imageStatus = reader.drawBMP(file, tft, 140, baseline+5);
  reader.printStatus(imageStatus);   // How'd we do?  

  ui.setTextColor(WX_WHITE, WX_BLACK);
  tft.setFont(&smallFont);
  ui.setTextAlignment(LEFT);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.drawString(baseX, baseline+25, "Sun");
  ui.setTextColor(WX_WHITE, WX_BLACK);
  ui.drawString(baseX, baseline+43, weather.getSunriseTime());
  ui.drawString(baseX, baseline+60, weather.getSunsetTime());

  ui.setTextAlignment(RIGHT);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.drawString(baseX+280, baseline+25, "Moon");
  ui.setTextColor(WX_WHITE, WX_BLACK);
  ui.drawString(baseX+280, baseline+43, weather.getMoonriseTime());
  ui.drawString(baseX+280, baseline+60, weather.getMoonsetTime());

}

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) {
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // discard any previously received packets, I made this change not sure if needed though
  }
  Serial.print("Transmit NTP Request ");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) { // Extending wait from 1500 to 2-3k seemed to avoid the sync problem, but now it doesn't help
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      if (!ntpSuccess) {
        ntpSuccess = true;
      }
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;   // This is in time_t format
    }
  }
  Serial.println("No NTP Response");
  if (ntpSuccess) {
    ntpSuccess = false;
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  int result;
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  result = Udp.beginPacket(address, 123); //NTP requests are to port 123
  result = Udp.write(packetBuffer, NTP_PACKET_SIZE);
  result = Udp.endPacket();
}
