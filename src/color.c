#include <pebble.h>
#include "pbmovies.h"

#ifdef PBL_COLOR

void set_menu_color(MenuLayer *menu) {
    menu_layer_set_normal_colors(menu, THEME_COLOR_MENU_NORMAL_BACKGROUND, THEME_COLOR_MENU_NORMAL_FOREGROUND);
    menu_layer_set_highlight_colors(menu, THEME_COLOR_MENU_HIGHLIGHT_BACKGROUND, THEME_COLOR_MENU_HIGHLIGHT_FOREGROUND);
}

#endif