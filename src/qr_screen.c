#include <pebble.h>
#include "pbmovies.h"
#include "preloader.h"
#include "qr_screen.h"

static struct QrCodeScreen {
#ifndef PBL_ROUND
    TextLayer *titleLayer;
#endif    
    Window *window;
    BitmapLayer *qrCodeLayer;
    GBitmap *qrCode;
} qrCodeScreen;

static void screen_load(Window *window) {

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    int startY=5;
#ifndef PBL_ROUND    
    //title layer
    qrCodeScreen.titleLayer = text_layer_create(GRect(0, startY, bounds.size.w, 24));
    text_layer_set_text_alignment(qrCodeScreen.titleLayer, GTextAlignmentCenter);
    text_layer_set_font(qrCodeScreen.titleLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text(qrCodeScreen.titleLayer, "Scan This Code");
    startY += 24;
    layer_add_child(window_layer, text_layer_get_layer(qrCodeScreen.titleLayer));
    //draw bitmap layer
#endif
   
    qrCodeScreen.qrCodeLayer = bitmap_layer_create(GRect(0, startY, bounds.size.w, bounds.size.h - startY));
    bitmap_layer_set_bitmap(qrCodeScreen.qrCodeLayer, qrCodeScreen.qrCode);
    bitmap_layer_set_alignment(qrCodeScreen.qrCodeLayer, GAlignCenter);

    layer_add_child(window_layer, bitmap_layer_get_layer(qrCodeScreen.qrCodeLayer));
}

static void screen_unload() {
    bitmap_layer_destroy(qrCodeScreen.qrCodeLayer);
    gbitmap_destroy(qrCodeScreen.qrCode);
#ifndef PBL_ROUND    
    text_layer_destroy(qrCodeScreen.titleLayer);
#endif    
    
    if (qrCodeScreen.window) {
        window_destroy(qrCodeScreen.window);
    }
    if (QR_CODE_BUFFER) {
        free(QR_CODE_BUFFER);
    }
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