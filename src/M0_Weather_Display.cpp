//
//  Copyright (C) 2017 Ronald Guest <http://about.me/ronguest>
//  Portions Copyright (c) 2015 by Daniel Eichhorn

#include "settings.h"

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

GfxUi ui = GfxUi(&tft);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

//declaring prototypes
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
typedef void (*ProgressCallback)(String fileName, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void showOverview();
void showForecastDetail();
void drawCurrentWeather();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
void drawForecast();
void drawAstronomy();
String getMeteoconIcon(String iconText);
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawSeparator(uint16_t y);
void sleepNow(int wakeup);
time_t getNtpTime();
void sendNTPpacket(IPAddress&);
void logMessage(String);

long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw

void setup(void) {
  time_t ntpTime;
  Serial.begin(115200);
  delay(1000);

  #ifdef WINC_EN
    pinMode(WINC_EN, OUTPUT);
    digitalWrite(WINC_EN, HIGH);
  #endif
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.println("FeatherWing TFT");
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");
  if (debug) Serial.println("Debug mode is ON");

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
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

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("SD failed!");
  }
  Serial.println("SD OK!");

  // Get current time using NTP
  Udp.begin(localPort);
  //setSyncProvider(getNtpTime);    // getNtpTime() connects to NTP server
  //setSyncInterval(300);           // 300 is in seconds, so sync time every 5 minutes
  ntpTime = getNtpTime();
  if (ntpTime != 0) {
    setTime(ntpTime);
  } else {
    Serial.println("Failed to set the initial time");
  }

  currentHour = -1;

  updateData();
}

void loop() {
  //int currentMinute;
  //int thisHour;

  // Check if we should update weather information
  if ((millis() - lastDownloadUpdate) > (1000 * UPDATE_INTERVAL_SECS)) {
    // Always display overview after an update
    showForecastText = false;
    updateData();
    lastDownloadUpdate = millis();
  }
  // If user touches screen, toggle between weather overview and the detailed forecast text
  if (ts.touched()) {
    showForecastText = !showForecastText;
    if (showForecastText) {
      // Show forecast text details in place of overview
      showForecastDetail();
    } else {
      // Switch back to default overview
      showOverview();
    }
  }
  delay(100);
}

// Update the internet based information and update screen
void updateData() {
  time_t local;
  time_t ntpTime;
  int thisHour;

  local = usCT.toLocal(now(), &tcr);
  thisHour = hour(local);

  wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_PWS);
  if (wunderground.errorMessage.length() > 1) logMessage(wunderground.errorMessage);
  // We only update the Forecast and Astronomy once an hour. They don't change much
  if (thisHour != currentHour) {
    currentHour = thisHour;
    wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_PWS);
    if (wunderground.errorMessage.length() > 1) logMessage(wunderground.errorMessage);
    wunderground.updateAstronomy(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_PWS);
    if (wunderground.errorMessage.length() > 1) logMessage(wunderground.errorMessage);
    // Try an NTP time sync so we don't get too far off
    ntpTime = getNtpTime();
    if (ntpTime != 0) {
      setTime(ntpTime);
    }
  }
  //lastUpdate = timeClient.getFormattedTime();
  //readyForWeatherUpdate = false;

  showOverview();
}

void showOverview() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);

  //drawTime();
  drawCurrentWeather();
  drawForecast();
  drawAstronomy();
}

void showForecastDetail() {
  int textLength;
  String text;
  int y = 30;
  int lineSize = 20;
  const int maxPerLine = 30;
  int period;
  int finalSpace;

  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(LEFT);

  // We show 4 periods which is 2 days + 2 nights
  for (period=0; period<4; period++) {
    //Serial.print("period: ");Serial.println(period);
    text = wunderground.getForecastText(period);
    //Serial.print("forecast text: ");Serial.println(text);
    textLength = text.length();
    Serial.print("Forecast length: "); Serial.println(textLength);

    // Draw the name of the period (e.g. "Monday" or "Monday Night")
    ui.drawString(0, y, wunderground.getForecastTitle(period));
    y += lineSize;
    int startPoint = 0;   // Position in text of next character to print
    while (startPoint < textLength) {
      // Find the last space in the next string we will print
      finalSpace = text.lastIndexOf(' ', startPoint + maxPerLine);
      if (finalSpace == -1 ) {
        // It's possible the final substring doesn't have a space
        finalSpace = textLength;
      }
      Serial.print("Final space: ");Serial.println(finalSpace);
      // If the first character is a space, skip it (happens due to line wrapping)
      if (text.indexOf(' ', startPoint) == startPoint) {
        startPoint++;
      }
      ui.drawString(0, y, text.substring(startPoint, finalSpace));
      y += lineSize;
      startPoint = finalSpace;
      Serial.print("Start point: ");Serial.println(startPoint);
    }
    y += lineSize;    // Add extra space between periods
  }
}

// draws current weather information
void drawCurrentWeather() {
  // Weather Icon
  String weatherIcon = getMeteoconIcon(wunderground.getTodayIcon());
  ui.drawBmp("/Icons/" + weatherIcon + ".bmp", 0, 0);

  // Weather Text
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(RIGHT);
  //ui.setTextSize(2);
  ui.drawString(220, 40, wunderground.getWeatherText());

  tft.setFont(&ArialRoundedMTBold_36);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  if (IS_METRIC) {
    degreeSign = "C";
  }
  String temp = wunderground.getCurrentTemp() + degreeSign;
  ui.drawString(220, 70, temp);
  drawSeparator(135);
}

// draws the three forecast columns
void drawForecast() {
  const int drop = 100;
  drawForecastDetail(10, drop, 0);
  drawForecastDetail(130, drop, 2);
  //drawForecastDetail(180, drop, 4);
  drawSeparator(drop + 65 + 10);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextAlignment(CENTER);
  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  ui.drawString(x + 45, y, day);

  tft.setFont(&ArialRoundedMTBold_36);
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(x + 40, y + 40, wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex));

  String weatherIcon = getMeteoconIcon(wunderground.getForecastIcon(dayIndex));
  //ui.drawBmp("/Minis/" + weatherIcon + ".bmp", x, y + 15);
  ui.drawBmp("/Icons/" + weatherIcon + ".bmp", x+0, y + 40);

}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  int moonAgeImage = 24 * wunderground.getMoonAge().toInt() / 30.0;
  ui.drawBmp("/Moon/" + String(moonAgeImage) + ".bmp", 120 - 30, 255);

  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextAlignment(LEFT);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.drawString(20, 270, "Sun");
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(20, 285, wunderground.getSunriseTime());
  ui.drawString(20, 300, wunderground.getSunsetTime());

  ui.setTextAlignment(RIGHT);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.drawString(220, 270, "Moon");
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(220, 285, wunderground.getMoonriseTime());
  ui.drawString(220, 300, wunderground.getMoonsetTime());

}

// Helper function, should be part of the weather station library and should disappear soon
String getMeteoconIcon(String iconText) {
  if (iconText == "F") return "cflur";
  if (iconText == "Q") return "crain";
  if (iconText == "W") return "csleet";
  if (iconText == "V") return "csnow";
  if (iconText == "S") return "cstorms";
  if (iconText == "B") return "clear";
  if (iconText == "Y") return "cloudy";
  if (iconText == "F") return "flurries";
  if (iconText == "M") return "fog";
  if (iconText == "E") return "hazy";
  if (iconText == "Y") return "mcloudy";
  if (iconText == "H") return "msunny";
  if (iconText == "H") return "pcloudy";
  if (iconText == "J") return "pysunny";
  if (iconText == "W") return "sleet";
  if (iconText == "R") return "rain";
  if (iconText == "W") return "snow";
  if (iconText == "B") return "sunny";
  if (iconText == "0") return "tstorms";
  return "unknown";
}
// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.fillRect(0, 140, 240, 45, ILI9341_BLACK);
  ui.drawString(120, 160, text);
  ui.drawProgressBar(10, 165, 240 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
}

// if you want separators, uncomment the tft-line
void drawSeparator(uint16_t y) {
   //tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
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

void logMessage(String s) {
  // Write string to SD card file
  File myFile;
  if (!debug) return;
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("debug.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to debug.txt...");
    myFile.println(s);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening debug.txt");
  }

}
