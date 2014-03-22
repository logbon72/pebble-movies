#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"

//struct SplsplashScreen splashScreen;

struct SplashScreen {
    Window *window;
    TextLayer *statusText;
    GBitmap *img;
    BitmapLayer *imgLayer;
    uint8_t loading;
} splashScreen;



static void splash_screen_select_handler(ClickRecognizerRef recognizer, void *context);
static void splash_click_config_provider(void *context);
static void splash_screen_load(Window *window);
static void splash_screen_select_handler(ClickRecognizerRef, void*);
static void splash_screen_unload(Window *window);
static void splash_send_init(void);

static void splash_screen_select_handler(ClickRecognizerRef recognizer, void *context) {
    if (!splashScreen.loading) {
        text_layer_set_text(splashScreen.statusText, LOADING_TEXT);
        //Tuplet * messages[] = {TupletInteger(APP_KEY_MSG_CODE, PB_MSG_OUT_INIT)};
        splash_send_init();
        splashScreen.loading = 1;
    }
}

static void splash_click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, splash_screen_select_handler);
}

static void splash_screen_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    window_set_background_color(window, GColorBlack);
    //window_set_fullscreen(window, true);

    splashScreen.statusText = text_layer_create((GRect) {
        .origin =
        { 0, 110}, .size =
        { bounds.size.w, 40}
    });
    text_layer_set_text(splashScreen.statusText, LOADING_TEXT);
    //text_layer_set_font(splashScreen.statusText, fonts_get_system_font(FONT_KEY_BITHAM_18_LIGHT_SUBSET));
    text_layer_set_background_color(splashScreen.statusText, GColorClear);
    text_layer_set_text_color(splashScreen.statusText, GColorWhite);
    text_layer_set_text_alignment(splashScreen.statusText, GTextAlignmentCenter);
    //add text
    layer_add_child(window_layer, text_layer_get_layer(splashScreen.statusText));
    //create image layer
    splashScreen.imgLayer = bitmap_layer_create(GRect(6, 30, 128, 70));
    //RESOURCE_ID_IMAGE_SPLASH
    if (!splashScreen.img) {
        splashScreen.img = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH_BLACK);
    }
    bitmap_layer_set_bitmap(splashScreen.imgLayer, splashScreen.img);
    layer_add_child(window_layer, bitmap_layer_get_layer(splashScreen.imgLayer));
    splashScreen.loading = 1;
}

static void splash_screen_unload(Window *window) {
    text_layer_destroy(splashScreen.statusText);
    bitmap_layer_destroy(splashScreen.imgLayer);
    gbitmap_destroy(splashScreen.img);
}

static void splash_send_init(void) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    if (iter == NULL) {
        return;
    }


    Tuplet msgCode = TupletInteger(APP_KEY_MSG_CODE, PB_MSG_OUT_INIT);
    dict_write_tuplet(iter, &msgCode);
    dict_write_end(iter);
    app_message_outbox_send();
}

void splash_screen_init(void) {
    splashScreen.window = window_create();
    window_set_click_config_provider(splashScreen.window, splash_click_config_provider);

    window_set_window_handlers(splashScreen.window, (WindowHandlers) {
        .load = splash_screen_load,
        .unload = splash_screen_unload,
    });
    const bool animated = true;

    window_stack_push(splashScreen.window, animated);
}

void splash_screen_set_status_text(char *text) {
    text_layer_set_text(splashScreen.statusText, text);
}

void splash_screen_hide() {
    splashScreen.loading = 0;
    window_stack_remove(splashScreen.window, true);
}

void splash_screen_set_loading(uint8_t loading){
    splashScreen.loading = loading;
}