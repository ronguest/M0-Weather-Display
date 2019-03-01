M0 Weather Display
========

* Current conditions come from Ambient Weather's site
* The forecast data is from Wunderground/Weather.com/IBM. For some reason starting around 3pm local time temperatureMax, wxPhraseLong, wxPhraseShort, windPhrase, windDirectional, uvIndex, thunder* etc return "null" for the first value in their respective arrays. Apparently 3pm is considered the end of "today's" day time so values for those types of fields are no longer returned. Special case handling is needed for this (e.g. don't want to show today's name as "null").
* The weather icons are stored on the SD card rather than being downloaded
* Currently using a Adafruit M0 Feather and 3.5" Adafruit TFT PID: 3651, 480x320. Formerly a 2.4" TFT, 320x240
* The project idea came from Daniel Eichorn https://github.com/squix78/esp8266-weather-station-color. The fonts and Gfx code are his.
* The fonts were created by http://oleddisplay.squix.ch/
