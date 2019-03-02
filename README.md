M0 Weather Display
========

* Current conditions come from Ambient Weather's site/API. Docs: https://www.ambientweather.com/api.html
* Forecast data is from Wunderground/Weather.com/IBM. For some reason starting around 3pm local time temperatureMax, wxPhraseLong, wxPhraseShort, windPhrase, windDirectional, uvIndex, thunder* etc return "null" for the first value in their respective arrays. Apparently 3pm is considered the end of "today's" day time so values for those types of fields are no longer returned. Special case handling is needed for this (e.g. don't want to show today's name as "null"). API docs are here: https://docs.google.com/document/d/1eKCnKXI9xnoMGRRzOL1xPCBihNV2rOet08qpE_gArAY/edit
* I think the new API from WU/IBM is for legacy users only. So those without a API key will have to use Dark Sky or some other provider. Given the changes I made it should be relatively straightforward to modify the code.
* Dark Sky API docs: https://darksky.net/dev/docs
* The weather and moon icons are stored on the SD card
* Currently using a Adafruit M0 Feather and 3.5" Adafruit TFT PID: 3651, 480x320. Formerly a 2.4" TFT, 320x240
* The project idea came from Daniel Eichorn https://github.com/squix78/esp8266-weather-station-color. The fonts and Gfx code are his.
* The fonts were created by http://oleddisplay.squix.ch/

* Dark Sky's API seems to require SSL which did not work initially. I had to use the Official Arduino IDE and their very convoluted process to update the WINC1500 firmware on the M0 Feather and then load a certificate specifically for api.darksky.net. I also added api.ambientweather.net's certfificate for testing purposes. I tried adding api.weather.com but the Arudion tool gave an RSA error on that one. So now the sketch is using SSL for Dark Sky and AW but not Weather.com

Dark Sky icon current possible values:  clear-day, clear-night, rain, snow, sleet, wind, fog, cloudy, partly-cloudy-day, or partly-cloudy-night.

Mappings for short SD file names
- clear-day = cday
- clear-night = cnight
- partly-cloudy-day = dpcloud
- partly-cloudy-night = npcloud
