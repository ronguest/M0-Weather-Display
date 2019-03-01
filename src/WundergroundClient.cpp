/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn with modifications for PWS by Ron Guest

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

#include <WiFi101.h>
#include "WundergroundClient.h"

extern "C" char *sbrk(int i);

File myFile;

bool usePM = false; // Set to true if you want to use AM/PM time disaply
bool isPM = false; // JJG added ///////////

WundergroundClient::WundergroundClient(boolean foo) {
}

void WundergroundClient::updateConditions(String device, String appKey, String apiKey) {
  isForecast = false;
  doUpdate("api.ambientweather.net", "/v1/devices/" + device + "?applicationKey=" + appKey + "&apiKey=" + apiKey + "&limit=1");
}

void WundergroundClient::updateForecast(String postalKey, String apiKey) {
  isForecast = true;
  doUpdate("api.weather.com", "/v3/wx/forecast/daily/5day?postalKey=" + postalKey + "&units=e&language=en-US&format=json&apiKey=" + apiKey);
}

void WundergroundClient::doUpdate(char server[], String url) {
  JsonStreamingParser parser;
  parser.setListener(this);
  WiFiClient client;
  // Red LED output on the M0 Feather
  const int ledPin = 13;

  Serial.print("Server: "); Serial.println(server);
  Serial.println("URL: " + url);
  digitalWrite(ledPin, HIGH);   // Turn on ledPin, it will stay on if we get an error
  if (!client.connect(server, 80)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("Requesting URL: "); Serial.println(server + url); Serial.flush();

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + server + "\r\n" +
               "Connection: close\r\n\r\n");

  char stack_dummy = 0;
  Serial.print("memory remaining is: "); Serial.println(&stack_dummy - sbrk(0));

  int retryCounter = 0;
  while(!client.available()) {
    delay(1000);
    retryCounter++;
    if (retryCounter > 15) {
      Serial.println(F("Retry timed out"));
      return;
    }
  }

  //int pos = 0;
  boolean isBody = false;
  char c;

  int size = 0;
  //client.setNoDelay(false);
  while(client.connected()) {
    while((size = client.available()) > 0) {
      c = client.read();
      //Serial.print(c);
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
      }
    }
  }
  client.stop();          // We're done, shut down the connection

  digitalWrite(ledPin, LOW);
}

void WundergroundClient::whitespace(char c) {
  //Serial.println(F("whitespace"));
}

void WundergroundClient::startDocument() {
  //Serial.println(F("start document"));
}

void WundergroundClient::key(String key) {
  //Serial.print("key: " + key);
  currentKey = String(key);
}

void WundergroundClient::value(String value) {
  //Serial.println(", value: " + value);

  if (currentKey == "windspeedmph") {
    windSpeed = value;
  }
  if (currentKey == "winddir") {
    windDir = value;
  }
  if (currentKey == "tempf") {
    currentTemp = value;
  }
  if (currentKey == "temperatureMax") {
    // Should only be null afer 3pm which is an arbitrary cut off by WU ?
    if (!value.equalsIgnoreCase("null")) {
      forecastHighTemp[currentForecastPeriod++] = value;
    }
  }
  if (currentKey == "temperatureMin") {
    forecastLowTemp[currentForecastPeriod++] = value;
  }
  if (currentKey == "iconCode") {
    forecastIcon[currentForecastPeriod++] = value.toInt();
  }
  if (currentKey == "narrative") {
    if (!value.equalsIgnoreCase("null")) {
      fcttext[currentForecastPeriod++] = value;
    }
  }
  if (currentKey == "humidity") {
    humidity = value;
  }
  if (currentKey == "baromrelin") {
    pressure = value + "mb";
  }
  if (currentKey == "dewPoint") {
    dewPoint = value;
  }
  if (currentKey == "dailyrainin") {
    precipitationToday = value + "in";
  }
  if (currentKey == "moonPhaseDay") {
    moonAge[currentForecastPeriod++] = value;
  }
  if (currentKey == "daypartName") {
    forecastTitle[currentForecastPeriod++] = value;
  }

  // *** I might need to use the below approach -- may handle evenings/night better???
  /*int dailyForecastPeriod = (currentForecastPeriod - 1) * 2;
  if (currentKey == "fahrenheit" && dailyForecastPeriod < MAX_FORECAST_PERIODS) {

      if (currentParent == "high") {
        forecastHighTemp[dailyForecastPeriod] = value;
      }
      if (currentParent == "low") {
        forecastLowTemp[dailyForecastPeriod] = value;
      }
  }*/

  // Prevent currentForecastPeriod going out of bounds (shouldn't happen but...)
  if (currentForecastPeriod >= MAX_FORECAST_PERIODS) {
    currentForecastPeriod = MAX_FORECAST_PERIODS - 1;
  }
}

void WundergroundClient::endArray() {
  //Serial.println("endArray");
}
void WundergroundClient::startArray() {
  //Serial.println("startArray");
  currentForecastPeriod = 0;
}

void WundergroundClient::startObject() {
  currentParent = currentKey;
  //Serial.println("startObject: " + currentParent);
}

void WundergroundClient::endObject() {
  //Serial.println("endObject: " + currentParent);
  currentParent = "";
}

void WundergroundClient::endDocument() {

}

String WundergroundClient::getHours() {
    if (localEpoc == 0) {
      return "--";
    }
    int hours = (getCurrentEpoch()  % 86400L) / 3600 + gmtOffset;
    if (hours < 10) {
      return "0" + String(hours);
    }
    return String(hours); // print the hour (86400 equals secs per day)

}
String WundergroundClient::getMinutes() {
    if (localEpoc == 0) {
      return "--";
    }
    int minutes = ((getCurrentEpoch() % 3600) / 60);
    if (minutes < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      return "0" + String(minutes);
    }
    return String(minutes);
}
String WundergroundClient::getSeconds() {
    if (localEpoc == 0) {
      return "--";
    }
    int seconds = getCurrentEpoch() % 60;
    if ( seconds < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      return "0" + String(seconds);
    }
    return String(seconds);
}
String WundergroundClient::getDate() {
  return date;
}
long WundergroundClient::getCurrentEpoch() {
  return localEpoc + ((millis() - localMillisAtUpdate) / 1000);
}

// JJG added ... /////////////////////////////////////////////////////////////////////////////////////////
String WundergroundClient::getMoonPctIlum() {
  return moonPctIlum;
}

String WundergroundClient::getMoonAge() {
  return moonAge[0];
}

String WundergroundClient::getMoonPhase() {
  return moonPhase;
}

String WundergroundClient::getSunriseTime() {
  return sunriseTime;
 }

String WundergroundClient::getSunsetTime() {
  return sunsetTime;
 }

String WundergroundClient::getMoonriseTime() {
  return moonriseTime;
 }

String WundergroundClient::getMoonsetTime() {
  return moonsetTime;
 }

String WundergroundClient::getWindSpeed() {
  return windSpeed;
 }

String WundergroundClient::getWindDir() {
  return windDir;
 }

 // end JJG add ////////////////////////////////////////////////////////////////////////////////////////////


String WundergroundClient::getCurrentTemp() {
  return currentTemp;
}

String WundergroundClient::getWeatherText() {
  return weatherText;
}

String WundergroundClient::getHumidity() {
  return humidity;
}

String WundergroundClient::getPressure() {
  return pressure;
}

String WundergroundClient::getDewPoint() {
  return dewPoint;
}

String WundergroundClient::getPrecipitationToday() {
  return precipitationToday;
}

String WundergroundClient::getTodayIcon() {
  return getMeteoconIcon(forecastIcon[0]);
}

/*int WundergroundClient::getTodayIconText() {
  return weatherIcon;
}*/

String WundergroundClient::getForecastIcon(int period) {
  return getMeteoconIcon(forecastIcon[period]);
}

String WundergroundClient::getForecastTitle(int period) {
  return forecastTitle[period];
}

String WundergroundClient::getForecastLowTemp(int period) {
  return forecastLowTemp[period];
}

String WundergroundClient::getForecastHighTemp(int period) {
  return forecastHighTemp[period];
}

String WundergroundClient::getForecastText(int period) {
  return fcttext[period];
}

// Converts the WU icon code to the file name for the M0
String WundergroundClient::getMeteoconIcon(int iconCode) {
  switch (iconCode){
  case 3:
    return "tstorms";
  case 4:
    return "tstorms";
  case 6:
    return "sleet";
  case 7:
    return "sleet";
  case 8:
    return "sleet";
  case 9:
    return "rain";
  case 10:
    return "sleet";
  case 11:
    return "rain";
  case 12:
    return "rain";
  case 13:
    return "flurries";
  case 14:
    return "snow";
  case 15:
    return "snow";
  case 16:
    return "snow";
  case 17:
    return "tstorms";
  case 18:
    return "sleet";
  case 19:
    return "fog";
  case 20:
    return "fog";
  case 21:
    return "hazy";
  case 22:
    return "hazy";
  case 25:
    return "sleet";
  case 26:
    return "cloudy";
  case 27:
    return "mcloudy";
  case 28:
    return "mcloudy";
  case 29:
    return "pcloudy";
  case 30:
    return "pcloudy";
  case 31:
    return "clear";
  case 32:
    return "sunny";
  case 33:
    return "pysunny";
  case 34:
    return "pysunny";
  case 35:
    return "tstorms";
  case 36:
    return "sunny";
  case 37:
    return "tstorms";
  case 38:
    return "tstorms";
  case 39:
    return "rain";
  case 40:
    return "rain";
  case 41:
    return "flurries";
  case 42:
    return "snow";
  case 43:
    return "snow";
  case 45:
    return "rain";
  case 46:
    return "snow";
  case 47:
    return "tstorms";
  default:
    return "unknown";
  }
}
