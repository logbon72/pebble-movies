
#include "pebble.h"

struct HomeScreen {
    Window *window;
    //TextLayer *text;
    MenuLayer *menu_layer;
    GBitmap *menuIcons[2];
    char *menuTexts[2];
} homeScreen;

#define HOME_MENU_SECTIONS 1
#define HOME_MENU_ROWS 2

void home_screen_init(void);