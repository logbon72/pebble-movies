#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"

void handle_start_app(void);
void handle_init_failed(const char *message);

static void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *msgCode = dict_find(iter, APP_KEY_MSG_CODE);
    int msgType = msgCode->value->int16;

    switch (msgType) {
        case PB_MSG_IN_INIT_FAILED:
            //Tuple *message = dict_find(iter, APP_KEY_MESSAGE);
            //handle_init_failed(message ? message->value->cstring : "");
            break;
        case PB_MSG_IN_START_APP:
            //handle_start_app();
            break;

        default:
            APP_LOG(APP_LOG_LEVEL_INFO, "Unknown message code received %d", msgType);

    }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
}

void app_message_init() {
    // Register message handlers
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    // Init buffers
    app_message_open(PB_INBOX_SIZE, PB_OUTBOX_SIZE);
    //fetch_msg();
}
//'const struct Tuplet * const' but argument is of type 'struct Tuple *'



//static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
//    text_layer_set_text(splashScreen.statusText, "Select");
//}
//
//static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
//    text_layer_set_text(splashScreen.statusText, "Up");
//}
//
//static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
//    text_layer_set_text(splashScreen.statusText, "Down");
//}
//
//static void click_config_provider(void *context) {
//    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
//    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
//    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
//}

void handle_init_failed(const char *message) {
    text_layer_set_text(splashScreen.statusText, message);
    splashScreen.loading = 0;
}

void handle_start_app() {
    splashScreen.loading = 0;
    window_stack_pop(true);
    //init home screen
}