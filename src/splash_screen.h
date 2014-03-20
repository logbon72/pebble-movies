#pragma once

struct SplashScreen {
    Window *window;
    TextLayer *statusText;
    GBitmap *img;
    BitmapLayer *imgLayer;
    uint8_t loading;
} splashScreen;


void splash_screen_init(void);