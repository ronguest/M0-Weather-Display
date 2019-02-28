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

#include <Adafruit_WINC1500.h>
#include "WundergroundClient.h"

extern "C" char *sbrk(int i);

File myFile;

bool usePM = false; // Set to true if you want to use AM/PM time disaply
bool isPM = false; // JJG added ///////////

WundergroundClient::WundergroundClient(boolean _isMetric) {
  isMetric = _isMetric;
}

// **********
// Example URL for the 10 day forecast: http://api.wunderground.com/api/APIKey/forecast10day/q/pws:KTXALLEN99.json
// **********

// For retrieving based on location
void WundergroundClient::updateConditions(String apiKey, String pws) {
  isForecast = false;
  doUpdate("v2/pws/observations/current?stationId=" + pws + "&format=json&units=e&apiKey=" + apiKey);
}

void WundergroundClient::updateForecast(String apiKey, String postal) {
  isForecast = true;
  doUpdate("v3/wx/forecast/daily/5day?postalKey=" + postal + "/forecast10day/lang:" + "S&units=e&language=en-US&format=json&apiKey=" + apiKey);
}

void WundergroundClient::doUpdate(String url) {
  JsonStreamingParser parser;
  parser.setListener(this);
  Adafruit_WINC1500Client client;
  const int httpPort = 443;
  const char* server = "api.weather.com";
  // Red LED output on the M0 Feather
  const int ledPin = 13;

  digitalWrite(ledPin, HIGH);   // Turn on ledPin, it will stay on if we get an error
  if (!client.connect(server, httpPort)) {
    myFile = SD.open("debug.txt", FILE_WRITE);
    if (myFile) {
      myFile.print(F("Failed to connect to: "));Serial.println(url);
      myFile.close();
    }
    Serial.println("connection failed");
    return;
  }

  Serial.print(F("Requesting URL: ")); Serial.println(server + '/' + url); Serial.flush();

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.weather.com\r\n" +
               "Connection: close\r\n\r\n");

  char stack_dummy = 0;
  Serial.print("memory remaining is: "); Serial.println(&stack_dummy - sbrk(0));

  int retryCounter = 0;
  while(!client.available()) {
    delay(1000);
    retryCounter++;
    if (retryCounter > 10) {
      myFile = SD.open("debug.txt", FILE_WRITE);
      if (myFile) {
        myFile.print(F("Retry timed out to: "));Serial.println(url);
        myFile.close();
      }
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

  Serial.print(F("isBody = "));Serial.println(isBody);
  if (!isBody) {
    myFile = SD.open("debug.txt", FILE_WRITE);
    if (myFile) {
      myFile.print("Was not isBody: ");myFile.println(url);
      myFile.close();
    }
  }
  digitalWrite(ledPin, LOW);
}

void WundergroundClient::whitespace(char c) {
  //Serial.println(F("whitespace"));
}

void WundergroundClient::startDocument() {
  Serial.println(F("start document"));
}

void WundergroundClient::value(String value) {
  if (currentKey == "moonPhaseDay") {
    moonAge = value;
  }

  if (currentKey == "moonPhase") {
    moonPhase = value;
  }

  if (currentKey == "windSpeed") {
    windSpeed = value;
  }

   if (currentKey == "windDirection") {
    windDir = value;
  }

  if (currentKey == "temp") {
    currentTemp = value;
  }

  if (currentKey == "icon") {
    if (isForecast && !isSimpleForecast && currentForecastPeriod < MAX_FORECAST_PERIODS) {
      //Serial.println(String(currentForecastPeriod) + ": " + value + ":" + currentParent);
      forecastIcon[currentForecastPeriod] = value;
    }
    if (!isForecast) {
      weatherIcon = value;
    }
  }
  weatherIcon = "unk";

  if (currentKey == "wxPhraseLong") {
    weatherText = value;
  }
  if (currentKey == "humidity") {
    humidity = value;
  }

  if (currentKey == "narrative" && currentForecastPeriod < MAX_FORECAST_PERIODS) {
      //Serial.println(String(currentForecastPeriod) + ": " + value);
      fcttext[currentForecastPeriod] = value;
  }
  // The detailed forecast period has only one forecast per day with low/high for both
  // night and day, starting at index 1.
/*  int dailyForecastPeriod = (currentForecastPeriod - 1) * 2;

  if (currentKey == "fahrenheit" && dailyForecastPeriod < MAX_FORECAST_PERIODS) {

      if (currentParent == "high") {
        forecastHighTemp[dailyForecastPeriod] = value;
      }
      if (currentParent == "low") {
        forecastLowTemp[dailyForecastPeriod] = value;
      }
  }
  if (currentKey == "celsius" && dailyForecastPeriod < MAX_FORECAST_PERIODS) {

      if (currentParent == "high") {
        //Serial.println(String(currentForecastPeriod)+ ": " + value);
        forecastHighTemp[dailyForecastPeriod] = value;
      }
      if (currentParent == "low") {
        forecastLowTemp[dailyForecastPeriod] = value;
      }
  }*/
}

void WundergroundClient::endArray() {

}


void WundergroundClient::startObject() {
  currentParent = currentKey;
}

void WundergroundClient::endObject() {
  currentParent = "";
}

void WundergroundClient::endDocument() {

}

void WundergroundClient::startArray() {

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
  return moonAge;
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
  return getMeteoconIcon(weatherIcon);
}

String WundergroundClient::getTodayIconText() {
  return weatherIcon;
}

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


String WundergroundClient::getMeteoconIcon(String iconText) {
  if (iconText == F("chanceflurries")) return "F";
  if (iconText == F("chancerain")) return "Q";
  if (iconText == F("chancesleet")) return "W";
  if (iconText == F("chancesnow")) return "V";
  if (iconText == F("chancetstorms")) return "S";
  if (iconText == F("clear")) return "B";
  if (iconText == F("cloudy")) return "Y";
  if (iconText == F("flurries")) return "F";
  if (iconText == F("fog")) return "M";
  if (iconText == F("hazy")) return "E";
  if (iconText == F("mostlycloudy")) return "Y";
  if (iconText == F("mostlysunny")) return "H";
  if (iconText == F("partlycloudy")) return "H";
  if (iconText == F("partlysunny")) return "J";
  if (iconText == F("sleet")) return "W";
  if (iconText == F("rain")) return "R";
  if (iconText == F("snow")) return "W";
  if (iconText == F("sunny")) return "B";
  if (iconText == F("tstorms")) return "0";

  if (iconText == F("nt_chanceflurries")) return "F";
  if (iconText == F("nt_chancerain")) return "7";
  if (iconText == F("nt_chancesleet")) return "#";
  if (iconText == F("nt_chancesnow")) return "#";
  if (iconText == F("nt_chancetstorms")) return "&";
  if (iconText == F("nt_clear")) return "2";
  if (iconText == F("nt_cloudy")) return "Y";
  if (iconText == F("nt_flurries")) return "9";
  if (iconText == F("nt_fog")) return "M";
  if (iconText == F("nt_hazy")) return "E";
  if (iconText == F("nt_mostlycloudy")) return "5";
  if (iconText == F("nt_mostlysunny")) return "3";
  if (iconText == F("nt_partlycloudy")) return "4";
  if (iconText == F("nt_partlysunny")) return "4";
  if (iconText == F("nt_sleet")) return "9";
  if (iconText == F("nt_rain")) return "7";
  if (iconText == F("nt_snow")) return "#";
  if (iconText == F("nt_sunny")) return "4";
  if (iconText == F("nt_tstorms")) return "&";

  return ")";
}
