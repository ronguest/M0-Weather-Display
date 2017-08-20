//
//   Copyright (C) 2017 Ronald Guest <http://about.me/ronguest>

char ssid[] = "SSID";     //  your network SSID (name)
char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)

// visit io.adafruit.com if you need to create an account,
// or if you need your Adafruit IO key.
#define IO_USERNAME    "AIO_username"
#define IO_KEY         "AIO_key"

// I am using a PWS (personal weather station) on wunderground.com.
// 1. Replace WAPI_KEY with your wunderground API key
// 2. Replace PWS_ID with the PWS ID you want to use, or...
//    or
//    you can switch to using a Country and State/City code
const String WUNDERGRROUND_API_KEY = F("WAPI_KEY");
const String WUNDERGROUND_PWS = "pws:PWD_ID";
//const String WUNDERGROUND_COUNTRY = "US";
//const String WUNDERGROUND_CITY = "FL/Royal_Palm_Beach";
