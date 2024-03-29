M0 Weather Display
========

* Current conditions come from Ambient Weather's site/API. Docs: https://www.ambientweather.com/api.html
* Forecast data is from Wunderground/Weather.com/IBM. For some reason starting around 3pm local time temperatureMax, wxPhraseLong, wxPhraseShort, windPhrase, windDirectional, uvIndex, thunder* etc return "null" for the first value in their respective arrays. Apparently 3pm is considered the end of "today's" day time so values for those types of fields are no longer returned. Special case handling is needed for this (e.g. don't want to show today's name as "null"). API docs are here: https://docs.google.com/document/d/1eKCnKXI9xnoMGRRzOL1xPCBihNV2rOet08qpE_gArAY/edit
* I think the new API from WU/IBM is for legacy users only. So those without a API key will have to use Dark Sky or some other provider. Given the changes I made it should be relatively straightforward to modify the code.
* The weather and moon icons are stored on the SD card
* Currently using a Adafruit M0 Feather and 3.5" Adafruit TFT PID: 3651, 480x320. Formerly a 2.4" TFT, 320x240
* The project idea came from Daniel Eichorn https://github.com/squix78/esp8266-weather-station-color. The fonts and Gfx code are his.
* The fonts were created by http://oleddisplay.squix.ch/
