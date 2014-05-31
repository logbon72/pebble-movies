#include <pebble.h>
#include "pbmovies.h"
#include "home.h"
#include "preloader.h"
#include "start.h"

#define HOME_MENU_SECTIONS 1
#define HOME_MENU_ROWS 2

static GBitmap *menuIcons[HOME_MENU_ROWS];
static SimpleMenuItem menuItems[HOME_MENU_ROWS];
static SimpleMenuSection menuSection[HOME_MENU_SECTIONS];

static struct HomeScreen {
    Window *window;
    //TextLayer *text;
    SimpleMenuLayer *menu_layer;
} homeScreen;


// Here we capture when a user selects a menu item

static void home_screen_select_handler(int index, void *ctx) {
    // Use the row to specify which item will receive the select action
    if (!bluetooth_connection_service_peek()) {
        preloader_init();
        preloader_set_no_connect();
        return;
    }
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
        return;
    }
    
    uint16_t msgId = index ? PB_MSG_OUT_GET_THEATRES : PB_MSG_OUT_GET_MOVIES;
    Tuplet msgCode = TupletInteger(APP_KEY_MSG_CODE, msgId);
    dict_write_tuplet(iter, &msgCode);

    Tuplet dOffset = TupletInteger(APP_KEY_DATE_OFFSET, get_date_offset());
    dict_write_tuplet(iter, &dOffset);

    dict_write_end(iter);
    app_message_outbox_send();
    preloader_init();
}

static void home_screen_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    //window_set_background_color(window, GColorBlack);

    menuIcons[0] = gbitmap_create_with_resource(RESOURCE_ID_ICON_FILM);
    menuIcons[1] = gbitmap_create_with_resource(RESOURCE_ID_ICON_THEATRE);

    menuItems[0] = (SimpleMenuItem){
        .title = "Movies",
        .icon = menuIcons[0],
        .callback = home_screen_select_handler,
    };

    menuItems[1] = (SimpleMenuItem){
        .title = "Theaters",
        .icon = menuIcons[1],
        .callback = home_screen_select_handler,
    };

    menuSection[0] = (SimpleMenuSection){
        .items = menuItems,
        .num_items = HOME_MENU_ROWS,
        .title = CurrentDateStr,
    };

    homeScreen.menu_layer = simple_menu_layer_create(bounds, window, menuSection, HOME_MENU_SECTIONS, NULL);

    // Add it to the window for display
    layer_add_child(window_layer, simple_menu_layer_get_layer(homeScreen.menu_layer));
}

static void home_screen_unload() {
    simple_menu_layer_destroy(homeScreen.menu_layer);
    for (uint16_t i = 0; i < ARRAY_LENGTH(menuIcons); i++) {
        gbitmap_destroy(menuIcons[i]);
    }
}

void home_screen_init(void) {
    homeScreen.window = window_create();
    //configure listeners
    //window_set_click_config_provider(homeScreen.window, splash_click_config_provider);

    window_set_window_handlers(homeScreen.window, (WindowHandlers) {
        .load = home_screen_load,
        .unload = home_screen_unload,
    });

    const bool animated = true;
    window_stack_push(homeScreen.window, animated);
}

void home_screen_deinit() {
    //    if (homeScreen.window) {
    //        window_destroy(homeScreen.window);
    //    }
}