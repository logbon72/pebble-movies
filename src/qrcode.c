#include <pebble.h>
#include "pbmovies.h"
#include "preloader.h"
#include "qrcode.h"

static struct QrCodeScreen {
    TextLayer *titleLayer;
    Window *window;
    BitmapLayer *qrCodeLayer;
    GBitmap *qrCode;
} qrCodeScreen;

static void screen_load(Window *window) {

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    //title layer
    qrCodeScreen.titleLayer = text_layer_create(GRect(0, 5, bounds.size.w, 24));
    text_layer_set_text_alignment(qrCodeScreen.titleLayer, GTextAlignmentCenter);
    text_layer_set_font(qrCodeScreen.titleLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(qrCodeScreen.titleLayer, "Scan This Code");
    //draw bitmap layer
    layer_add_child(window_layer, text_layer_get_layer(qrCodeScreen.titleLayer));


    qrCodeScreen.qrCodeLayer = bitmap_layer_create(GRect(0, 34, bounds.size.w, bounds.size.h - 33));
    bitmap_layer_set_bitmap(qrCodeScreen.qrCodeLayer, qrCodeScreen.qrCode);
    bitmap_layer_set_alignment(qrCodeScreen.qrCodeLayer, GAlignCenter);

    layer_add_child(window_layer, bitmap_layer_get_layer(qrCodeScreen.qrCodeLayer));

}

static void screen_unload() {
    bitmap_layer_destroy(qrCodeScreen.qrCodeLayer);
    gbitmap_destroy(qrCodeScreen.qrCode);
    text_layer_destroy(qrCodeScreen.titleLayer);
}

static void screen_appear(Window *window) {
    //do lights 
    light_enable_interaction();
    //do vibrate
    vibes_short_pulse();
    preloader_set_hidden(window);
}

void qr_code_init() {

    qrCodeScreen.window = window_create();
    qrCodeScreen.qrCode = gbitmap_create_with_data(QR_CODE_BUFFER);

    window_set_window_handlers(qrCodeScreen.window, (WindowHandlers) {
        .load = screen_load,
        .unload = screen_unload,
        .appear = screen_appear,
    });


    window_stack_push(qrCodeScreen.window, true);
}