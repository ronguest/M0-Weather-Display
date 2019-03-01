//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#include <WiFi101.h>
#include "WeatherClient.h"

extern "C" char *sbrk(int i);

File myFile;

bool usePM = false; // Set to true if you want to use AM/PM time disaply
bool isPM = false; // JJG added ///////////

WeatherClient::WeatherClient(boolean foo) {
}

void WeatherClient::updateConditions(String device, String appKey, String apiKey) {
  doUpdate("api.ambientweather.net", "/v1/devices/" + device + "?applicationKey=" + appKey + "&apiKey=" + apiKey + "&limit=1");
}

void WeatherClient::updateForecast(String postalKey, String apiKey) {
  doUpdate("api.weather.com", "/v3/wx/forecast/daily/5day?postalKey=" + postalKey + "&units=e&language=en-US&format=json&apiKey=" + apiKey);
}

void WeatherClient::doUpdate(char server[], String url) {
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

void WeatherClient::key(String key) {
  //Serial.print("key: " + key);
  currentKey = String(key);
}

void WeatherClient::value(String value) {
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
  if (currentKey == "sunriseTimeLocal") {
    Serial.println("sunriseTime: " + value);
    sunriseTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
  }
  if (currentKey == "sunsetTimeLocal") {
    sunsetTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
  }
  if (currentKey == "moonriseTimeLocal") {
      moonriseTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
    }
  if (currentKey == "moonsetTimeLocal") {
      moonsetTime[currentForecastPeriod++] = value.substring(value.indexOf('T')+1,value.lastIndexOf(':'));
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

void WeatherClient::endArray() {
  //Serial.println("endArray");
}
void WeatherClient::startArray() {
  //Serial.println("startArray");
  currentForecastPeriod = 0;
}

void WeatherClient::startObject() {
  currentParent = currentKey;
  //Serial.println("startObject: " + currentParent);
}

void WeatherClient::endObject() {
  //Serial.println("endObject: " + currentParent);
  currentParent = "";
}

void WeatherClient::endDocument() {

}

String WeatherClient::getMoonAge() {
  return moonAge[0];
}

String WeatherClient::getSunriseTime() {
  return sunriseTime[0];
 }

String WeatherClient::getSunsetTime() {
  return sunsetTime[0];
 }

String WeatherClient::getMoonriseTime() {
  return moonriseTime[0];
 }

String WeatherClient::getMoonsetTime() {
  return moonsetTime[0];
 }

String WeatherClient::getWindSpeed() {
  return windSpeed;
 }

String WeatherClient::getWindDir() {
  return windDir;
 }

String WeatherClient::getCurrentTemp() {
  return currentTemp;
}

String WeatherClient::getWeatherText() {
  return weatherText;
}

String WeatherClient::getHumidity() {
  return humidity;
}

String WeatherClient::getPressure() {
  return pressure;
}

String WeatherClient::getDewPoint() {
  return dewPoint;
}

String WeatherClient::getPrecipitationToday() {
  return precipitationToday;
}

String WeatherClient::getTodayIcon() {
  return getMeteoconIcon(forecastIcon[0]);
}

String WeatherClient::getForecastIcon(int period) {
  return getMeteoconIcon(forecastIcon[period]);
}

String WeatherClient::getForecastTitle(int period) {
  return forecastTitle[period];
}

String WeatherClient::getForecastLowTemp(int period) {
  return forecastLowTemp[period];
}

String WeatherClient::getForecastHighTemp(int period) {
  return forecastHighTemp[period];
}

String WeatherClient::getForecastText(int period) {
  return fcttext[period];
}

void WeatherClient::whitespace(char c) {
  //Serial.println(F("whitespace"));
}

void WeatherClient::startDocument() {
  //Serial.println(F("start document"));
}

// Converts the WU icon code to the file name for the M0
String WeatherClient::getMeteoconIcon(int iconCode) {
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
