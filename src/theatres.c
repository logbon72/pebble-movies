#include <pebble.h>
#include "pbmovies.h"
#include "theatres.h"

#define THEATRE_FLD_SIZE_ID 8
#define THEATRE_FLD_SIZE_NAME 48
#define THEATRE_FLD_SIZE_ADDR 32
#define THEATRE_FLD_SIZE_DISTANCE 8

struct TheatreRecord {
    char id[THEATRE_FLD_SIZE_ID];
    char name[THEATRE_FLD_SIZE_NAME];
    char address[THEATRE_FLD_SIZE_ADDR];
    char distance[THEATRE_FLD_SIZE_DISTANCE];
} currentTheatre;

static void set_current_theatre(uint16_t theatreIndex) {
    if (theatreIndex >= theatresUI.total) {
        theatreIndex = 0;
    }

    theatresUI.currentIndex = theatreIndex;

    //should icons be shown?
    if (theatreIndex > 0) {
        action_bar_layer_set_icon(theatresUI.actionBar, BUTTON_ID_UP, theatresUI.upIcon);
    } else {
        action_bar_layer_clear_icon(theatresUI.actionBar, BUTTON_ID_UP);
    }

    if (theatreIndex < theatresUI.total - 1) {
        action_bar_layer_set_icon(theatresUI.actionBar, BUTTON_ID_DOWN, theatresUI.downIcon);
    } else {
        action_bar_layer_clear_icon(theatresUI.actionBar, BUTTON_ID_DOWN);
    }


    get_data_at(THEATRES_LIST, theatreIndex, 0, currentTheatre.id, THEATRE_FLD_SIZE_ID);
    get_data_at(THEATRES_LIST, theatreIndex, 1, currentTheatre.name, THEATRE_FLD_SIZE_NAME);
    get_data_at(THEATRES_LIST, theatreIndex, 2, currentTheatre.address, THEATRE_FLD_SIZE_ADDR);
    get_data_at(THEATRES_LIST, theatreIndex, 3, currentTheatre.distance, THEATRE_FLD_SIZE_DISTANCE);

    //set address - id, name, address, distance_m
    text_layer_set_text(theatresUI.address, currentTheatre.address);
    text_layer_set_text(theatresUI.name, currentTheatre.name);
    text_layer_set_text(theatresUI.distance, currentTheatre.distance);

}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (currentTheatre.id) {
        if (theatresUI.currentMode == TheatreUIModeTheatres) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get theatre movies for theatre ID: %s", currentTheatre.id);
        } else {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get showtimes for theatre ID: %s and Movie ID: %s", currentTheatre.id, theatresUI.currentMovie);
        }
    }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (theatresUI.currentIndex > 0) {
        set_current_theatre(theatresUI.currentIndex - 1);
    }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (theatresUI.currentIndex < theatresUI.total - 1) {
        set_current_theatre(theatresUI.currentIndex + 1);
    }
}

static void theatre_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void theatres_screen_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    uint16_t usableWidth = bounds.size.w - 25 - 10;
    uint16_t paddingSide = 10;
    //address
    theatresUI.address = text_layer_create(GRect(paddingSide, 5, usableWidth, 50));
    text_layer_set_font(theatresUI.address, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.address));
    //theatre name
    theatresUI.name = text_layer_create(GRect(paddingSide, 51, usableWidth, 80));
    text_layer_set_font(theatresUI.name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.name));


    //distance
    theatresUI.distance = text_layer_create(GRect(paddingSide, 131, usableWidth, 15));
    text_layer_set_font(theatresUI.distance, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(theatresUI.distance, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.distance));

    //now add action bar!!!
    theatresUI.actionBar = action_bar_layer_create();
    action_bar_layer_add_to_window(theatresUI.actionBar, theatresUI.window);

    //icons
    theatresUI.upIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_UP_WHITE);
    theatresUI.downIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_DOWN_WHITE);

    if (theatresUI.currentMode == TheatreUIModeTheatres) {
        theatresUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_MOVIE_BLACK);
    } else {
        theatresUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_SHOWTIME_BLACK);
    }

    action_bar_layer_set_icon(theatresUI.actionBar, BUTTON_ID_SELECT, theatresUI.selectIcon);
    action_bar_layer_set_click_config_provider(theatresUI.actionBar, theatre_click_config_provider);
    //action_bar_layer_set_background_color(theatresUI.actionBar, GColorWhite);

    //then set current
    set_current_theatre(theatresUI.currentIndex);
}

static void theatres_screen_unload() {
    theatresUI.currentMovie = NULL;
    gbitmap_destroy(theatresUI.upIcon);
    gbitmap_destroy(theatresUI.downIcon);
    gbitmap_destroy(theatresUI.selectIcon);
    text_layer_destroy(theatresUI.address);
    text_layer_destroy(theatresUI.distance);
    text_layer_destroy(theatresUI.name);
    //text_layer_destroy(theatresUI.titleBar);
    action_bar_layer_destroy(theatresUI.actionBar);
}

void theatres_screen_initialize(int total, enum TheatreUiMode mode, char *movieId) {

    theatresUI.total = total;
    theatresUI.currentMode = mode;
    if (theatresUI.currentMovie) {
        theatresUI.currentMovie = movieId;
    } else {
        theatresUI.currentMovie = NULL;
    }
    //ui.currentMode = mode;

    theatresUI.currentIndex = 0;
    theatresUI.window = window_create();

    window_set_window_handlers(theatresUI.window, (WindowHandlers) {
        .load = theatres_screen_load,
        .unload = theatres_screen_unload
    });


    const bool animated = true;
    window_stack_push(theatresUI.window, animated);
}

