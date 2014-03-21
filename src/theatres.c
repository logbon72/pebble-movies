#include <pebble.h>
#include "pbmovies.h"
#include "theatres.h"

#define THEATRE_EXPECTED_LENGTH 4

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

    //    if (theatresUI.current) {
    //        free(theatresUI.current);
    //    }

    char *duplicated = strdup(theatresUI.theatres[theatreIndex]);
    //    APP_LOG(APP_LOG_LEVEL_INFO, "Data at index %d is %s", theatreIndex, duplicated);
    //    return;
    int length = 0;
    theatresUI.current = str_split(duplicated, DELIMITER_FIELD, &length);
    //    free(duplicated);

    if (length < THEATRE_EXPECTED_LENGTH) {
        return; // set_current_theatre(++theatreIndex);
    }

    //set title text
    //char titleText[30];
    //strcat(titleText, theatresUI.currentMode == THEATRE_UI_MODE_THEATRES ? "Theatres (" : "Movie Theatres (");
    //    char tmpStore[6];

    //    snprintf(tmpStore, 6, "%d/", theatreIndex + 1);
    //    strcat(titleText, tmpStore);
    //    snprintf(tmpStore, 6, "%d)", theatresUI.total);
    //    strcat(titleText, tmpStore);

    //    text_layer_set_text(theatresUI.titleBar, theatresUI.currentMode == THEATRE_UI_MODE_THEATRES ? "Theatres" : "Movie Theatres");
    //set address - id, name, address, distance_m
    text_layer_set_text(theatresUI.address, theatresUI.current[2]);
    text_layer_set_text(theatresUI.name, theatresUI.current[1]);
    if (strcmp(theatresUI.current[3], " ")) {
        //char distance[20];
        //strcat(distance, theatresUI.current[3]);
        //strcat(distance, " away");
        text_layer_set_text(theatresUI.distance, theatresUI.current[3]);
    } else {
        text_layer_set_text(theatresUI.distance, "");
    }

}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (theatresUI.currentMode == THEATRE_UI_MODE_THEATRES) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get theatre movies for theatre ID: %s", theatresUI.current[0]);
    } else {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get showtimes for theatre ID: %s and Movie ID: %s", theatresUI.current[0], theatresUI.currentMovie);
    }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    set_current_theatre(theatresUI.currentIndex - 1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    set_current_theatre(theatresUI.currentIndex + 1);
}

static void theatre_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

//static void update_title_bar(Layer *layer, GContext* ctx) {
//    GRect bounds = layer_get_bounds(layer);
//    graphics_context_set_fill_color(ctx, GColorBlack);
//    graphics_context_set_text_color(ctx, GColorWhite);
//    uint16_t r = 10;
//    graphics_fill_rect(ctx, bounds, r, GCornersBottom);
//}

static void theatres_screen_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    uint16_t usableWidth = bounds.size.w - 30 - 10;
    uint16_t paddingSide = 10;
    //title bar
    //    theatresUI.titleBar = text_layer_create(GRect(paddingSide, 0, usableWidth, 25)); //height 25;
    //    text_layer_set_text_alignment(theatresUI.titleBar, GTextAlignmentCenter);
    //    text_layer_set_text_color(theatresUI.titleBar, GColorWhite);
    //    text_layer_set_background_color(theatresUI.titleBar, GColorBlack);
    //    text_layer_set_font(theatresUI.titleBar, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    //    Layer *titleBarLayer = text_layer_get_layer(theatresUI.titleBar);
    //layer_set_update_proc(titleBarLayer, update_title_bar);
    //    layer_add_child(window_layer, titleBarLayer);

    //address
    theatresUI.address = text_layer_create(GRect(paddingSide, 5, usableWidth, 40));
    text_layer_set_font(theatresUI.address, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.address));

    //theatre name
    theatresUI.name = text_layer_create(GRect(paddingSide, 41, usableWidth, 60));
    text_layer_set_font(theatresUI.name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.name));

    //distance
    theatresUI.distance = text_layer_create(GRect(paddingSide, 102, usableWidth, 15));
    text_layer_set_font(theatresUI.distance, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.distance));

    //now add action bar!!!
    theatresUI.actionBar = action_bar_layer_create();
    action_bar_layer_add_to_window(theatresUI.actionBar, theatresUI.window);
    theatresUI.upIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_UP);
    theatresUI.downIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_DOWN);

    if (theatresUI.currentMode == THEATRE_UI_MODE_THEATRES) {
        theatresUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_MOVIE);
    } else {
        theatresUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_SHOWTIME);
    }

    action_bar_layer_set_icon(theatresUI.actionBar, BUTTON_ID_SELECT, theatresUI.selectIcon);
    action_bar_layer_set_click_config_provider(theatresUI.actionBar, theatre_click_config_provider);
    //action_bar_layer_set_background_color(theatresUI.actionBar, GColorWhite);

    //then set current
    set_current_theatre(theatresUI.currentIndex);
}

static void theatres_screen_unload() {
    theatresUI.current = NULL;
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

void theatres_screen_initialize(char **theatres, int total, enum TheatreUiMode mode, char *movieId) {

//    if (theatresUI.theatres) {
//        free(theatresUI.theatres);
//        theatresUI.theatres = NULL;
//    }

    theatresUI.theatres = theatres;
    theatresUI.total = total;
    theatresUI.currentMode = mode;
    if (theatresUI.currentMovie) {
        theatresUI.currentMovie = movieId;
    } else {
        theatresUI.currentMovie = NULL;
    }
    //ui.currentMode = mode;

    theatresUI.window = window_create();

    window_set_window_handlers(theatresUI.window, (WindowHandlers) {
        .load = theatres_screen_load,
        .unload = theatres_screen_unload
    });


    const bool animated = true;

    window_stack_push(theatresUI.window, animated);

}