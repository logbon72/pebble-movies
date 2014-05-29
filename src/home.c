#include <pebble.h>
#include "pbmovies.h"
#include "home.h"
#include "preloader.h"

static struct HomeScreen {
    Window *window;
    //TextLayer *text;
    MenuLayer *menu_layer;
    GBitmap *menuIcons[2];
    char *menuTexts[2];
} homeScreen;

#define HOME_MENU_SECTIONS 1
#define HOME_MENU_ROWS 2

static uint16_t home_screen_num_sections(MenuLayer *menu_layer, void *data) {
    return HOME_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this

static uint16_t home_screen_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return HOME_MENU_ROWS;
}

// A callback is used to specify the height of the section header

static int16_t home_screen_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is

static void home_screen_draw_header(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    // Determine which section we're working with
    menu_cell_basic_header_draw(ctx, cell_layer, CurrentDateStr);
}

// Here we capture when a user selects a menu item

static void home_screen_select_handler(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
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
    uint16_t msgId = cell_index->row ? PB_MSG_OUT_GET_THEATRES : PB_MSG_OUT_GET_MOVIES;
    Tuplet msgCode = TupletInteger(APP_KEY_MSG_CODE, msgId);
    dict_write_tuplet(iter, &msgCode);
    dict_write_end(iter);
    app_message_outbox_send();
    preloader_init();
}

// This is the menu item draw callback where you specify what each item should look like

static void home_screen_cell_drawer(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    menu_cell_basic_draw(ctx, cell_layer, homeScreen.menuTexts[cell_index->row % 2], NULL, homeScreen.menuIcons[cell_index->row % 2]);
}

static void home_screen_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    //window_set_background_color(window, GColorBlack);

    homeScreen.menu_layer = menu_layer_create(bounds);

    homeScreen.menuIcons[0] = gbitmap_create_with_resource(RESOURCE_ID_ICON_FILM);
    homeScreen.menuIcons[1] = gbitmap_create_with_resource(RESOURCE_ID_ICON_THEATRE);
    homeScreen.menuTexts[0] = "Movies";
    homeScreen.menuTexts[1] = "Theatres";

    // Set all the callbacks for the menu layer

    menu_layer_set_callbacks(homeScreen.menu_layer, NULL, (MenuLayerCallbacks) {
        .get_num_sections = home_screen_num_sections,
        .get_num_rows = home_screen_num_rows,
        .get_header_height = home_screen_header_height_callback,
        .draw_header = home_screen_draw_header,
        .draw_row = home_screen_cell_drawer,
        .select_click = home_screen_select_handler,
    });

    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(homeScreen.menu_layer, window);

    // Add it to the window for display
    layer_add_child(window_layer, menu_layer_get_layer(homeScreen.menu_layer));
}

static void home_screen_unload() {
    menu_layer_destroy(homeScreen.menu_layer);
    for (uint16_t i = 0; i < ARRAY_LENGTH(homeScreen.menuIcons); i++) {
        gbitmap_destroy(homeScreen.menuIcons[i]);
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