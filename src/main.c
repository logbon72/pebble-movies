#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"
#include "home.h"

//static Window *window;
//static TextLayer *text_layer;
static void init(void) {
    app_message_init();
    splash_screen_init();
}



int main(void) {
    init();
    app_event_loop();
    home_screen_deinit();
}
