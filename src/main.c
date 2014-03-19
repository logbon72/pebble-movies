#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"

//static Window *window;
//static TextLayer *text_layer;


static void init(void) {
    app_message_init();
    splash_screen_init();
}

static void deinit(void) {
    //window_destroy(splashScreen.window);
}

int main(void) {
    init();
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Max Inbox: %lu", app_message_inbox_size_maximum());
    app_event_loop();
    deinit();
}
