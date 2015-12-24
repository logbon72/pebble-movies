#include <pebble.h>
#include <pebble_fonts.h>
#include <gcolor_definitions.h>
#include "pbmovies.h"
#include "theatres.h"
#include "preloader.h"

#define HEIGHT_NAME 75
#define HEIGHT_ADDRESS 32
#define HEIGHT_DISTANCE 14
#define MARGIN 5
#define ROUND_SIZE (HEIGHT_ADDRESS+HEIGHT_NAME+HEIGHT_DISTANCE+MARGIN*2)

struct TheatreUI {
    Window *window;
#ifndef PBL_ROUND
    ActionBarLayer *actionBar;
    GBitmap *upIcon;
    GBitmap *downIcon;
    GBitmap *selectIcon;
#endif

#ifndef PBL_RECT
    ContentIndicator *indicator;
    Layer *upIndicatorLayer;
    Layer *downIndicatorLayer;
#endif
    //TextLayer *titleBar;
    TextLayer *address;
    TextLayer *name;
    TextLayer *distance;
    enum TheatreUiMode currentMode;
    //char **theatres;
    uint16_t currentIndex;
    uint16_t total;
    //char **current;
    char *currentMovie;
};

static struct TheatreUI theatresUI;

static void set_current_theatre(uint16_t theatreIndex) {
    if (theatreIndex >= theatresUI.total) {
        theatreIndex = 0;
    }

    theatresUI.currentIndex = theatreIndex;

    //should icons be shown?
#ifndef PBL_ROUND
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
#endif

#ifndef PBL_RECT
    content_indicator_set_content_available(theatresUI.indicator, ContentIndicatorDirectionUp, theatreIndex > 0);
    content_indicator_set_content_available(theatresUI.indicator, ContentIndicatorDirectionDown, theatreIndex < theatresUI.total - 1);
#endif



    get_data_at(THEATRES_BUFFER, theatreIndex, 0, currentTheatre.id, THEATRE_FLD_SIZE_ID);
    get_data_at(THEATRES_BUFFER, theatreIndex, 1, currentTheatre.name, THEATRE_FLD_SIZE_NAME);
    get_data_at(THEATRES_BUFFER, theatreIndex, 2, currentTheatre.address, THEATRE_FLD_SIZE_ADDR);
    get_data_at(THEATRES_BUFFER, theatreIndex, 3, currentTheatre.distance, THEATRE_FLD_SIZE_DISTANCE);

    //set address - id, name, address, distance_m
    text_layer_set_text(theatresUI.address, currentTheatre.address);
    text_layer_set_text(theatresUI.name, currentTheatre.name);
    text_layer_set_text(theatresUI.distance, currentTheatre.distance);

}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (currentTheatre.id) {
        if (theatresUI.currentMode == TheatreUIModeTheatres) {
            //APP_LOG(APP_LOG_LEVEL_DEBUG, "Next, get theatre movies for theatre ID: %s", );
            if (send_message_with_string(PB_MSG_OUT_GET_THEATRE_MOVIES, APP_KEY_THEATRE_ID, currentTheatre.id, 0, NULL)) {
                preloader_init();
            }
        } else {
            load_showtimes_for_movie_theatre();
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
#ifndef PBL_ROUND

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
    action_bar_layer_set_background_color(theatresUI.actionBar, PBL_IF_COLOR_ELSE(THEME_COLOR_BACKGROUND_SECONDARY, GColorBlack));
    action_bar_layer_add_to_window(theatresUI.actionBar, theatresUI.window);

    //icons
    theatresUI.upIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_UP);
    theatresUI.downIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_DOWN);

    if (theatresUI.currentMode == TheatreUIModeTheatres) {
        theatresUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_MOVIE);
    } else {
        theatresUI.selectIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_A_BAR_SHOWTIME);
    }

    action_bar_layer_set_icon(theatresUI.actionBar, BUTTON_ID_SELECT, theatresUI.selectIcon);
    action_bar_layer_set_click_config_provider(theatresUI.actionBar, theatre_click_config_provider);

    //then set current
    set_current_theatre(theatresUI.currentIndex);
}
#endif

#ifndef PBL_RECT

//static void draw_background_cirlce(Layer *layer, GContext* ctx) {
//    GRect bounds = layer_get_bounds(layer);
//    graphics_context_set_fill_color(ctx, THEME_COLOR_BACKGROUND_SECONDARY);
//    graphics_fill_circle(ctx, GRectCenter(bounds), bounds.size.w / 2);
//}

static void theatres_screen_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect boundsMain = layer_get_bounds(window_layer);
    GRect bounds = GRectCenterIn(ROUND_SIZE, ROUND_SIZE, boundsMain);


    //set for circle background
    window_set_background_color(window, THEME_COLOR_BACKGROUND_SECONDARY);
    uint16_t usableWidth = bounds.size.w - MARGIN * 2;
    uint16_t startX = bounds.origin.x + MARGIN;
    uint16_t startY = bounds.origin.y + MARGIN * 2;

    //address
    theatresUI.address = text_layer_create(GRect(startX, startY, usableWidth, HEIGHT_ADDRESS));
    text_layer_set_font(theatresUI.address, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(theatresUI.address, GTextAlignmentCenter);
    text_layer_set_overflow_mode(theatresUI.address, GTextOverflowModeWordWrap);
    text_layer_set_background_color(theatresUI.address, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.address));
    text_layer_enable_screen_text_flow_and_paging(theatresUI.address, 2);


    startY += HEIGHT_ADDRESS;
    //theatre name
    theatresUI.name = text_layer_create(GRect(startX, startY, usableWidth, HEIGHT_NAME));
    text_layer_set_font(theatresUI.name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(theatresUI.name, GTextAlignmentCenter);
    text_layer_set_overflow_mode(theatresUI.name, GTextOverflowModeWordWrap);
    text_layer_set_background_color(theatresUI.name, GColorClear);
    text_layer_set_text_color(theatresUI.name, THEME_COLOR_TEXT_SECONDARY);
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.name));
    text_layer_enable_screen_text_flow_and_paging(theatresUI.name, 2);

    startY += HEIGHT_NAME;

    //distance
    theatresUI.distance = text_layer_create(GRect(startX, startY, usableWidth, HEIGHT_DISTANCE));
    text_layer_set_font(theatresUI.distance, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(theatresUI.distance, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(theatresUI.distance));
    text_layer_set_background_color(theatresUI.distance, GColorClear);
    startY += HEIGHT_DISTANCE;

    //content indicator
    theatresUI.indicator = content_indicator_create();

    theatresUI.upIndicatorLayer = layer_create(GRect(0, 0, boundsMain.size.w, STATUS_BAR_LAYER_HEIGHT));
    theatresUI.downIndicatorLayer = layer_create(GRect(0, boundsMain.size.h - STATUS_BAR_LAYER_HEIGHT,
            boundsMain.size.w, STATUS_BAR_LAYER_HEIGHT));
    layer_add_child(window_layer, theatresUI.upIndicatorLayer);
    layer_add_child(window_layer, theatresUI.downIndicatorLayer);


    const ContentIndicatorConfig downConfig = (ContentIndicatorConfig){
        .layer = theatresUI.downIndicatorLayer,
        .times_out = false,
        .alignment = GAlignCenter,
        .colors =
        {
            .foreground = THEME_COLOR_TEXT_SECONDARY,
            .background = THEME_COLOR_BACKGROUND_SECONDARY
        }
    };


    const ContentIndicatorConfig upConfig = (ContentIndicatorConfig){
        .layer = theatresUI.upIndicatorLayer,
        .times_out = false,
        .alignment = GAlignCenter,
        .colors =
        {
            .foreground = THEME_COLOR_TEXT_SECONDARY,
            .background = THEME_COLOR_BACKGROUND_SECONDARY
        }
    };

    content_indicator_configure_direction(theatresUI.indicator, ContentIndicatorDirectionUp, &upConfig);
    content_indicator_configure_direction(theatresUI.indicator, ContentIndicatorDirectionDown, &downConfig);

    window_set_click_config_provider(theatresUI.window, theatre_click_config_provider);
    //then set current
    set_current_theatre(theatresUI.currentIndex);

}
#endif

static void theatres_screen_unload() {
    theatresUI.currentMovie = NULL;
#ifndef PBL_ROUND
    gbitmap_destroy(theatresUI.upIcon);
    gbitmap_destroy(theatresUI.downIcon);
    gbitmap_destroy(theatresUI.selectIcon);
    action_bar_layer_destroy(theatresUI.actionBar);
#endif

#ifndef PBL_RECT
    content_indicator_destroy(theatresUI.indicator);
    layer_destroy(theatresUI.upIndicatorLayer);
    layer_destroy(theatresUI.downIndicatorLayer);
#endif    


    text_layer_destroy(theatresUI.address);
    text_layer_destroy(theatresUI.distance);
    text_layer_destroy(theatresUI.name);
    //text_layer_destroy(theatresUI.titleBar);

    if (theatresUI.window) {
        window_destroy(theatresUI.window);
    }
    free(THEATRES_BUFFER);
}

//static void theatre_screen_appear(Window* window) {
//    ///APP_LOG(APP_LOG_LEVEL_DEBUG, "Theatre Window Loaded");
//}

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
        .unload = theatres_screen_unload,
        .appear = preloader_set_hidden,
    });


    const bool animated = true;
    window_stack_push(theatresUI.window, animated);
}


