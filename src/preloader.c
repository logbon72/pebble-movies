#include <pebble.h>
#include "pbmovies.h"
#include "preloader.h"

#define MAX_NUM_CIRCLES 9
#define RADIUS_INC 3
#define RADIUS_INITIAL 3
#define ANIMATION_TIMEOUT 100

static Layer *square_layer;
static GBitmap *statusBarIcon;
//static const char *STATUS_TEXT;
//static const char *PRESS_BACK ="Press Back Again";
static char *LOADING_TEXT = "Loading...";
static char *TIME_OUT = "Connection Timed Out!";

static struct PreloaderScreen {
    Window *window;
    TextLayer *statusText;
    uint8_t isOn;
    AppTimer *timer;
} preloader;
// Timers can be canceled with `app_timer_cancel()`
//static AppTimer *timer;

static int circleCount = 1;

static void update_square_layer(Layer *layer, GContext* ctx) {

    graphics_context_set_stroke_color(ctx, GColorWhite);
    for (int i = 0; i < circleCount; i++) {
        graphics_draw_circle(ctx, GPoint(70, 50), RADIUS_INITIAL + (i * RADIUS_INC));
    }

    if (++circleCount > MAX_NUM_CIRCLES) {
        circleCount = 1;
    }
    //graphics_context_set_fill_color(ctx, GColorBlack);
    //gpath_draw_outline(ctx, square_path);
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
    layer_destroy(square_layer);
    //window_destroy(window);
    app_timer_cancel(preloader.timer);
    text_layer_destroy(preloader.statusText);
    gbitmap_destroy(statusBarIcon);
    preloader.isOn = 1;
}

//static void pop_preloader(void *c) {
//    //window_stack_pop(true);
//}

static void preloader_appear(Window *window) {
    const uint32_t timeout_ms = ANIMATION_TIMEOUT;
    if(preloader.isOn){
        preloader.timer = app_timer_register(timeout_ms, timer_callback, NULL);
    }
//    else{
//        preloader.timer = app_timer_register(timeout_ms, pop_preloader, NULL);
//    }
    
}

static void preloader_load(Window *window) {
    window_set_background_color(window, GColorBlack);
    window_set_status_bar_icon(window, statusBarIcon);
    Layer *window_layer = window_get_root_layer(preloader.window);

    if (!statusBarIcon) {
        statusBarIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_STATUS_BAR);
    }

    GRect bounds = layer_get_bounds(window_layer);
    square_layer = layer_create(bounds);
    layer_set_update_proc(square_layer, update_square_layer);
    layer_add_child(window_layer, square_layer);

    preloader.statusText = text_layer_create(GRect(0, 80, bounds.size.w, bounds.size.h - 110));
    
    text_layer_set_background_color(preloader.statusText, GColorClear);
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

void preloader_set_timed_out() {
    preloader_set_is_on(1);
    preloader_set_status(TIME_OUT);
}

void preloader_set_is_on(uint8_t isOn) {
    preloader.isOn = isOn;
}

void preloader_stop() {
    preloader.isOn = 0;
    //if(preloader.window == window_stack_get_top_window()){
    //window_stack_pop(true);
    // }
}

void preloader_set_hidden(Window* window){
    preloader_set_is_on(0);
    window_stack_remove(preloader.window, true);
}

