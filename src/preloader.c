#include <pebble.h>
#include "pbmovies.h"
#include "preloader.h"

#define ANIMATION_TIMEOUT 300
#define STROKE_SIZE 20
#define PROGRESS_LAYER_WIDTH 100
#define PROGRESS_LAYER_HEIGHT 5
#define PROGRESS_STEP 5
#define STROKE_WIDTH 2
#define TEXT_HEIGHT 22

static Layer *progress_layer;
static char *LOADING_TEXT = "Loading...";
static char *TIME_OUT = "Connection Timed Out!";

static struct PreloaderScreen {
    Window *window;
    TextLayer *statusText;
    uint8_t isOn;
    AppTimer *timer;
} preloader;

static int progressPercent = 0;

static void timer_callback(void *context);

static void update_progress_layer(Layer *layer, GContext* ctx) {
    GRect bounds = layer_get_bounds(layer);

    if (!preloader.isOn) {
        progressPercent = 0;
    }

    graphics_context_set_fill_color(ctx, THEME_COLOR_BACKGROUND_PRIMARY);
    graphics_context_set_stroke_color(ctx, THEME_COLOR_BACKGROUND_PRIMARY);
    graphics_context_set_stroke_width(ctx, 1);


    graphics_draw_round_rect(ctx, bounds, 2);
    GRect rec = bounds;
    rec.size.w = (int16_t) ((PROGRESS_LAYER_WIDTH * progressPercent / 100));
    graphics_fill_rect(ctx, rec, 2, GCornersAll);
}

static void timer_cancel() {
    if (preloader.timer) {
        app_timer_cancel(preloader.timer);
        preloader.timer = NULL;
    }
}

static void next_timer() {
    if (preloader.isOn) {
        preloader.timer = app_timer_register(ANIMATION_TIMEOUT, timer_callback, NULL);
    }
}

static void timer_callback(void *context) {
    preloader_set_progress(progressPercent + PROGRESS_STEP, false);
    next_timer();
}

static void unload(Window *w) {
    //gpath_destroy(square_path);
    if (progress_layer) {
        layer_destroy(progress_layer);
    }
    //window_destroy(window);
    timer_cancel();
    preloader.timer = NULL;
    text_layer_destroy(preloader.statusText);
    progressPercent = 0;
    //gbitmap_destroy(statusBarIcon);
}

static void preloader_appear(Window *window) {
    next_timer();
}

static void preloader_load(Window *window) {
    //window_set_background_color(window, THEME_COLOR_BACKGROUND_PRIMARY);
    Layer *window_layer = window_get_root_layer(preloader.window);

    GRect bounds = layer_get_bounds(window_layer);
    GRect p_layer_bound = GRectCenterIn(PROGRESS_LAYER_WIDTH, PROGRESS_LAYER_HEIGHT, bounds);
    p_layer_bound.origin.y -= 10;
    progress_layer = layer_create(p_layer_bound);

    layer_set_update_proc(progress_layer, update_progress_layer);
    layer_add_child(window_layer, progress_layer);


    int width = bounds.size.w;
    preloader.statusText = text_layer_create(GRect(0, p_layer_bound.origin.y + p_layer_bound.size.h + 10, width, TEXT_HEIGHT));

    //text_layer_set_background_color(preloader.statusText, THEME_COLOR_BACKGROUND_PRIMARY);
    text_layer_set_text_color(preloader.statusText, THEME_COLOR_BACKGROUND_PRIMARY);
    text_layer_set_text_alignment(preloader.statusText, GTextAlignmentCenter);
    text_layer_set_font(preloader.statusText, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_overflow_mode(preloader.statusText, GTextOverflowModeWordWrap);

    layer_add_child(window_layer, text_layer_get_layer(preloader.statusText));
    preloader_set_loading();
}

void preloader_init() {
    preloader.window = window_create();
    //STATUS_TEXT = text;

    preloader.isOn = 1;
    preloader.timer = NULL;
    window_set_window_handlers(preloader.window, (WindowHandlers) {
        .unload = unload,
        .appear = preloader_appear,
        .load = preloader_load
    });

    window_stack_push(preloader.window, true);
}

void preloader_set_progress(int progress, bool cancelTimer) {
    progressPercent = progress > 100 ? 0 : progress;
    layer_mark_dirty(progress_layer);
    if(cancelTimer){
        timer_cancel();
    }
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
    window_stack_remove(preloader.window, false);
    if (preloader.window) {
        window_destroy(preloader.window);
    }
    preloader.window = NULL;
}

