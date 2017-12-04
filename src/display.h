//
//  Copyright (C) 2017 Ronald Guest <http://about.me/ronguest>

// ***** Only one of these should be set - HX8357 is the large TFT display, ILI9341 is the original
#define ILI9341 1
//#define HX8357 1

#ifdef HX8357
#define WX_BLACK HX8357_BLACK
#define WX_CYAN HX8357_CYAN
#define WX_BLUE HX8357_BLUE
#define WX_WHITE HX8357_WHITE
#endif
#ifdef ILI9341
#define WX_BLACK ILI9341_BLACK
#define WX_CYAN ILI9341_CYAN
#define WX_BLUE ILI9341_BLUE
#define WX_WHITE ILI9341_WHITE
#endif
