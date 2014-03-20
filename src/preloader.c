#include <pebble.h>
#include "pbmovies.h"
#include "preloader.h"

#define MAX_NUM_CIRCLES 9
#define RADIUS_INC 3
#define RADIUS_INITIAL 3
#define ANIMATION_TIMEOUT 100
static Layer *square_layer;
static GBitmap *statusBarIcon;
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

    const uint32_t timeout_ms = ANIMATION_TIMEOUT;
    preloader.timer = app_timer_register(timeout_ms, timer_callback, NULL);
}

static void unload(Window *w) {
    //gpath_destroy(square_path);
    layer_destroy(square_layer);
    //window_destroy(window);
    app_timer_cancel(preloader.timer);
    text_layer_destroy(preloader.statusText);
    gbitmap_destroy(statusBarIcon);
    preloader.isOn = 0;
}

void preloader_init(char *text) {
    preloader.window = window_create();
    window_set_background_color(preloader.window, GColorBlack);
    window_set_status_bar_icon(preloader.window, statusBarIcon);
    Layer *window_layer = window_get_root_layer(preloader.window);

    if (!statusBarIcon) {
        statusBarIcon = gbitmap_create_with_resource(RESOURCE_ID_ICON_STATUS_BAR);
    }


    GRect bounds = layer_get_bounds(window_layer);
    square_layer = layer_create(bounds);
    layer_set_update_proc(square_layer, update_square_layer);
    layer_add_child(window_layer, square_layer);

    //square_path = gpath_create(&SQUARE_POINTS);
    //gpath_move_to(square_path, grect_center_point(&bounds));

    window_set_window_handlers(preloader.window, (WindowHandlers) {
        .unload = unload,
    });



    preloader.statusText = text_layer_create(GRect(0, 85, bounds.size.w, bounds.size.h - 110));
    text_layer_set_text(preloader.statusText, text);
    text_layer_set_background_color(preloader.statusText, GColorClear);
    text_layer_set_text_color(preloader.statusText, GColorWhite);
    text_layer_set_text_alignment(preloader.statusText, GTextAlignmentCenter);
    text_layer_set_font(preloader.statusText, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(preloader.statusText));


    const bool animated = true;
    window_stack_push(preloader.window, animated);
    const uint32_t timeout_ms = ANIMATION_TIMEOUT;
    preloader.timer = app_timer_register(timeout_ms, timer_callback, NULL);
    preloader.isOn = 1;
}
