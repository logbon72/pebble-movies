#include <pebble.h>
#include "pbmovies.h"
#include "preloader.h"

#define ANIMATION_TIMEOUT 100
#define STROKE_SIZE 20
#define SQUARE_LENGTH 100
#define ANGLE_STEPS 30;
#define TEXT_HEIGHT 22

static Layer *square_layer;
static char *LOADING_TEXT = "Loading...";
static char *TIME_OUT = "Connection Timed Out!";

static struct PreloaderScreen {
    Window *window;
    TextLayer *statusText;
    uint8_t isOn;
    AppTimer *timer;
} preloader;

static int currentAngle = 0;

static void update_square_layer(Layer *layer, GContext* ctx) {

    GRect bounds = layer_get_bounds(layer);

    if (!preloader.isOn) {
        currentAngle = 360;
    } else {
        currentAngle += ANGLE_STEPS;
    }
    graphics_context_set_fill_color(ctx, THEME_COLOR_OUTLINE_PRIMARY);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFillCircle, STROKE_SIZE, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(currentAngle));

    if (currentAngle >= 360) {
        currentAngle = 0;
    }
}

static void timer_callback(void *context) {
    layer_mark_dirty(square_layer);
    if (preloader.isOn) {
        const uint32_t timeout_ms = ANIMATION_TIMEOUT;
        preloader.timer = app_timer_register(timeout_ms, timer_callback, NULL);
    }
}

static void unload(Window *w) {
    //gpath_destroy(square_path);
    if (square_layer) {
        layer_destroy(square_layer);
    }
    //window_destroy(window);
    if (preloader.timer) {
        app_timer_cancel(preloader.timer);
    }
    preloader.timer = NULL;
    text_layer_destroy(preloader.statusText);
    currentAngle = 0;
    //gbitmap_destroy(statusBarIcon);
    preloader.isOn = 1;
}

static void preloader_appear(Window *window) {
    const uint32_t timeout_ms = ANIMATION_TIMEOUT;
    if (preloader.isOn) {
        preloader.timer = app_timer_register(timeout_ms, timer_callback, NULL);
    }
}

static void preloader_load(Window *window) {
    window_set_background_color(window, THEME_COLOR_BACKGROUND_PRIMARY);
    Layer *window_layer = window_get_root_layer(preloader.window);

    GRect bounds = layer_get_bounds(window_layer);
    square_layer = layer_create(GRectCenterIn(SQUARE_LENGTH, SQUARE_LENGTH, bounds));

    layer_set_update_proc(square_layer, update_square_layer);
    layer_add_child(window_layer, square_layer);


    int width = bounds.size.w;
    preloader.statusText = text_layer_create(GRectCenterIn(width, TEXT_HEIGHT, bounds));

    text_layer_set_background_color(preloader.statusText, THEME_COLOR_BACKGROUND_PRIMARY);
    text_layer_set_text_color(preloader.statusText, GColorWhite);
    text_layer_set_text_alignment(preloader.statusText, GTextAlignmentCenter);
    text_layer_set_font(preloader.statusText, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_overflow_mode(preloader.statusText, GTextOverflowModeWordWrap);

    layer_add_child(window_layer, text_layer_get_layer(preloader.statusText));
    preloader_set_loading();
}

void preloader_init() {
    preloader.window = window_create();
    //STATUS_TEXT = text;

    window_set_window_handlers(preloader.window, (WindowHandlers) {
        .unload = unload,
        .appear = preloader_appear,
        .load = preloader_load
    });


    preloader.isOn = 1;
    const bool animated = true;
    window_stack_push(preloader.window, animated);
}

void preloader_set_status(char *text) {
    text_layer_set_text(preloader.statusText, text);
}

void preloader_set_loading() {
    preloader_set_status(LOADING_TEXT);
}

void preloader_set_no_connect() {
    preloader_set_status("No BT Connection");
    preloader_set_is_on(0);
}

void preloader_set_timed_out() {
    preloader_set_is_on(0);
    preloader_set_status(TIME_OUT);
}

void preloader_set_is_on(uint8_t isOn) {
    preloader.isOn = isOn;
}

void preloader_stop() {
    preloader.isOn = 0;
}

void preloader_set_hidden(Window* window) {
    preloader_set_is_on(0);
    //    APP_LOG(APP_LOG_LEVEL_INFO, "Popping preloader, %p", preloader.window);
    window_stack_remove(preloader.window, false);
    //    APP_LOG(APP_LOG_LEVEL_INFO, "Popped");
    if (preloader.window) {
        window_destroy(preloader.window);
    }
    preloader.window = NULL;
}

