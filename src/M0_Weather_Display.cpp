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

WeatherClient weather(true);

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
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawSeparator(uint16_t y);
void sleepNow(int wakeup);
time_t getNtpTime();
void sendNTPpacket(IPAddress&);
void todayDetail(int baseline);

long lastDownloadUpdate = -(1000L * UPDATE_INTERVAL_SECS)-1;    // Forces initial screen draw

void setup(void) {
  time_t ntpTime;
  Serial.begin(115200);
  delay(1000);

  //Configure pins for Adafruit ATWINC1500 Feather
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
}

void loop() {
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

  weather.updateConditions(AW_DEVICE, AW_APP_KEY, AW_API_KEY);
  // We only update the Forecast once an hour. They don't change much
  if (thisHour != currentHour) {
    currentHour = thisHour;
    weather.updateForecast(WUNDERGROUND_POSTAL_KEY, WUNDERGRROUND_API_KEY);
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
  tft.fillScreen(WX_BLACK);
  tft.setFont(&smallFont);

  drawTime();
  drawCurrentWeather();
  todayDetail(150);
  drawForecast();
  drawAstronomy();
}

void todayDetail(int baseline) {
  int textLength;
  String text;
  int maxLines = 4;
  int finalSpace;
  int startPoint = 0;   // Position in text of next character to print

  tft.setFont(&smallFont);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(LEFT);
  local = usCT.toLocal(now(), &tcr);
  hours = hour(local);

  int y = baseline;
  // Starting at 5pm show the forecast for the evening/night instead of the daytime forecast
  text = weather.getForecastText(hours >= 17 ? 1 : 0);   // Period 0 is daytime, period 1 is the night forecast
  textLength = text.length();
  Serial.print("Today detail length: "); Serial.println(textLength);
  while ((startPoint < textLength) && (maxLines > 0)){
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
    ui.drawString(10, y, text.substring(startPoint, finalSpace));
    y += lineSize;
    startPoint = finalSpace;
    Serial.print("Start point: ");Serial.println(startPoint);
    maxLines--;
  }
}

void showForecastDetail() {
  int textLength;
  String text;
  int y = 30;
  int period;
  int finalSpace;

  tft.fillScreen(WX_BLACK);
  tft.setFont(&smallFont);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(LEFT);

  // We show 4 periods which is 2 days + 2 nights
  for (period=0; period<4; period++) {
    //Serial.print("period: ");Serial.println(period);
    text = weather.getForecastText(period);
    //Serial.print("forecast text: ");Serial.println(text);
    textLength = text.length();
    Serial.print("Forecast length: "); Serial.println(textLength);

    // Draw the name of the period (e.g. "Monday" or "Monday Night")
    ui.drawString(0, y, weather.getForecastTitle(period));
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
      ui.drawString(10, y, text.substring(startPoint, finalSpace));
      y += lineSize;
      startPoint = finalSpace;
      Serial.print("Start point: ");Serial.println(startPoint);
    }
    y += lineSize;    // Add extra space between periods
  }
}

// Draws date and time
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
  //tft.setFont(&largeFont);
  if (hours >= 12) {
    hours = (hours > 12) ? hours - 12 : hours;
    ampm = "pm";
  }
  String timeS = String(hours) + ":" + (minutes < 10 ?"0" : "" ) + String(minutes) + ampm;
  //ui.drawString(120, 56, timeS);
  ui.drawString(150, 40, timeS);
  //drawSeparator(65);
}

// draws current weather information
void drawCurrentWeather() {
  // Weather Icon
  String weatherIcon = weather.getCurrentIcon();
  // WU no longer provides a current weather icon
//  ui.drawBmp("/Icons/" + weatherIcon + ".bmp", 20, 50);

  // Weather Text
  tft.setFont(&smallFont);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(RIGHT);
//  ui.drawString(220, 40, weather.getWeatherText());
  //ui.drawString(220, 40, weather.getWeatherText());
  ui.drawString(240, 60, weather.getWeatherText());

  tft.setFont(&largeFont);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  ui.setTextAlignment(RIGHT);
  String degreeSign = "F";
  String temp = weather.getCurrentTemp() + degreeSign;
  //ui.drawString(220, 70, temp);
//  ui.drawString(220, 70, temp);
  ui.drawString(200, 100, temp);
  //drawSeparator(135);
}

// draws the three forecast columns
void drawForecast() {
  //const int drop = 140;
  const int drop = 260;
//  drawForecastDetail(10, drop, 0);
//  drawForecastDetail(130, drop, 2);
  drawForecastDetail(30, drop, 0);
  drawForecastDetail(200, drop, 1);
  //drawSeparator(drop + 65 + 10);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  ui.setTextColor(WX_CYAN, WX_BLACK);
  tft.setFont(&smallFont);
  ui.setTextAlignment(CENTER);
  // evens are day time (0, 2, ...), odds are night time so * 2 to get next day rather than night
  String day = weather.getForecastDayOfWeek( dayIndex * 2).substring(0, 3);
  day.toUpperCase();
  ui.drawString(x + 45, y, day);

  tft.setFont(&largeFont);
  ui.setTextColor(WX_WHITE, WX_BLACK);
  ui.drawString(x + 40, y + 40, weather.getForecastLowTemp(dayIndex) + "|" + weather.getForecastHighTemp(dayIndex));

  String weatherIcon = weather.getForecastIcon(dayIndex);
  //ui.drawBmp("/Minis/" + weatherIcon + ".bmp", x, y + 15);
  //ui.drawBmp("/Icons/" + weatherIcon + ".bmp", x+0, y + 40);
  ui.drawBmp("/Icons/" + weatherIcon + ".bmp", x+0, y + 40);
}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  //int moonAgeImage = 24 * weather.getMoonAge().toInt();
  int baseline = 410;   // Place at the bottom
  int baseX = 20;
//  ui.drawBmp("/Moon/" + String(moonAgeImage) + ".bmp", 120 - 30, baseline);
  String moonFile;

  moonFile = "/Moon/" + weather.getMoonAge() + ".bmp";
  Serial.println("Load moon file: " + moonFile);
  ui.drawBmp(moonFile, 140, baseline+5);

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

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  ui.setTextAlignment(CENTER);
  ui.setTextColor(WX_CYAN, WX_BLACK);
  tft.fillRect(0, 140, 240, 45, WX_BLACK);
  ui.drawString(120, 160, text);
  ui.drawProgressBar(10, 165, 240 - 20, 15, percentage, WX_WHITE, WX_BLUE);
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
