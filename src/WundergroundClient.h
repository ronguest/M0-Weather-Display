/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

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

#pragma once

#include <JsonListener.h>
#include <JsonStreamingParser.h>
#include <SPI.h>
#include <SD.h>

#define MAX_FORECAST_PERIODS 24  // Changed from 7 to 12 to support 6 day / 2 screen forecast (Neptune)

class WundergroundClient: public JsonListener {
  private:
    String currentKey;
    String currentParent = "";
    long localEpoc = 0;
    int gmtOffset = 1;
    long localMillisAtUpdate;
    String date = "-";
    String currentTemp;
    String moonPctIlum;  // not used
    String moonAge[MAX_FORECAST_PERIODS];      // make this a long?
    String moonPhase;
    String sunriseTime[MAX_FORECAST_PERIODS];
    String sunsetTime[MAX_FORECAST_PERIODS];
    String moonriseTime[MAX_FORECAST_PERIODS];
    String moonsetTime[MAX_FORECAST_PERIODS];
    String windSpeed;
    String windDir;
    int weatherIcon;
    String weatherText;
    String humidity;
    String pressure;
    String dewPoint;
    String precipitationToday;
    void doUpdate(char server[], String url);

    // forecast
    boolean isForecast = false;
    boolean isSimpleForecast = true;
    int currentForecastPeriod;
    int forecastIcon [MAX_FORECAST_PERIODS];
    String forecastTitle [MAX_FORECAST_PERIODS];
    String forecastLowTemp [MAX_FORECAST_PERIODS];
    String forecastHighTemp [MAX_FORECAST_PERIODS];
    String fcttext [MAX_FORECAST_PERIODS];


  public:
    WundergroundClient(boolean foo);
    void updateConditions(String device, String appKey, String apiKey);
    void updateForecast(String postalKey, String apiKey);

    // JJG added
    String getHours();
    String getMinutes();
    String getSeconds();
    String getDate();
    // JJG added ... ///////////////////function name to string ////////////////////////////
    String getMoonPctIlum();
    String getMoonAge();
    String getMoonPhase();
    String getSunriseTime();
    String getSunsetTime();
    String getMoonriseTime();
    String getMoonsetTime();
    String getWindSpeed();
    String getWindDir();
    long getCurrentEpoch();
    String getCurrentTemp();

    String getTodayIcon();

    //int getTodayIconText();

    String getMeteoconIcon(int iconCode);

    String getWeatherText();

    String getHumidity();

    String getPressure();

    String getDewPoint();

    String getPrecipitationToday();

    String getForecastIcon(int period);

    String getForecastTitle(int period);

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
