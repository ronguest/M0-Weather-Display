//
//  Copyright (C) 2017-2019 Ronald Guest <http://ronguest.net>
//

#pragma once

#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include <SPI.h>
#include <SD.h>

#define MAX_FORECAST_PERIODS 24  // Changed from 7 to 12 to support 6 day / 2 screen forecast (Neptune)

class WeatherClient: public JsonListener {
  private:
    String currentKey;
    String currentParent = "";
    String currentTemp;
    int currentForecastPeriod;
    int forecastIcon [MAX_FORECAST_PERIODS];
    String forecastTitle [MAX_FORECAST_PERIODS];
    String forecastDayOfWeek [MAX_FORECAST_PERIODS];
    String forecastLowTemp [MAX_FORECAST_PERIODS];
    String forecastHighTemp [MAX_FORECAST_PERIODS];
    String fcttext [MAX_FORECAST_PERIODS];
    String moonAge[MAX_FORECAST_PERIODS];
    String sunriseTime[MAX_FORECAST_PERIODS];
    String sunsetTime[MAX_FORECAST_PERIODS];
    String moonriseTime[MAX_FORECAST_PERIODS];
    String moonsetTime[MAX_FORECAST_PERIODS];
    String windSpeed;
    String windDir;
    String currentIcon;
    int weatherIcon;
    String weatherText;
    String humidity;
    String pressure;
    String dewPoint;
    String precipitationToday;
    void doUpdate(int port, char server[], String url);

  public:
    WeatherClient(boolean foo);
    void updateConditions(String device, String appKey, String apiKey, String dsApiKey, String dsLatLon);
    void updateForecast(String postalKey, String apiKey);

    String getCurrentIcon();
    String getMoonAge();
    String getSunriseTime();
    String getSunsetTime();
    String getMoonriseTime();
    String getMoonsetTime();
    String getWindSpeed();
    String getWindDir();
    String getCurrentTemp();
    String getTodayIcon();
    String getMeteoconIcon(int iconCode);
    String getWeatherText();
    String getHumidity();
    String getPressure();
    String getDewPoint();
    String getPrecipitationToday();
    String getForecastIcon(int period);
    String getForecastTitle(int period);
    String getForecastDayOfWeek(int period);
    String getForecastLowTemp(int period);
    String getForecastHighTemp(int period);
    String getForecastText(int period);
    virtual void whitespace(char c);
    virtual void startDocument();
    virtual void key(String key);
    virtual void value(String value);
    virtual void endArray();
    virtual void endObject();
    virtual void endDocument();
    virtual void startArray();
    virtual void startObject();
};
