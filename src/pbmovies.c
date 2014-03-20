#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"
#include "home.h"
#include "theatres.h"
#include "preloader.h"
#define MSG_CODE_NO_WAIT (uint8_t) 6000

static void handle_start_app(void);
static void handle_init_failed(const char *message);
static void handle_data_received(uint8_t,uint8_t, uint8_t, char *);
static void close_wait(void *);

char *messageBuffer;
AppTimer *inboxWaitTimer;
uint8_t currentWaiter = MSG_CODE_NO_WAIT;

static void close_wait(void *context) {
    if (currentWaiter == MSG_CODE_NO_WAIT) {
        handle_data_received(currentWaiter, 0, 0, "");
    }

    if (inboxWaitTimer) {
        app_timer_cancel(inboxWaitTimer);
    }
}

static void handle_data_received(uint8_t msgCode,uint8_t page, uint8_t totalPages, char *data) {

    if(page !=1 && page != totalPages && !(currentWaiter || currentWaiter != msgCode)){
        APP_LOG(APP_LOG_LEVEL_INFO, "Message discarded, wait is over already");
        return;
    }
    
    if (!messageBuffer) {
        messageBuffer = strdup(data);
    } else {
        char *tmp = strdup(messageBuffer);
        free(messageBuffer);
        size_t newLen = strlen(tmp) + strlen(data) + 1;
        messageBuffer = malloc(sizeof (char*) * newLen);
        strcat(messageBuffer, tmp);
        strcat(messageBuffer, data);
        free(tmp);
    }
    free(data);

    if (page == totalPages) {
        if (inboxWaitTimer) {
            app_timer_cancel(inboxWaitTimer);
        }

        currentWaiter = MSG_CODE_NO_WAIT;
        //next thing...
        
        char **dataRecords = str_split(messageBuffer, DELIMITER_RECORD);
        free(messageBuffer);
        window_stack_pop(false);
        switch(msgCode){
            case PB_MSG_IN_THEATRES:
                theatres_screen_init(dataRecords, THEATRE_UI_MODE_THEATRES, NULL);
                break;
            
            default:
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d needs no handling", msgCode);
        }       
        
    } else {
        //some more messages expected, wait
        currentWaiter = msgCode;
        inboxWaitTimer = app_timer_register(MSG_INTERVAL_WAIT_MS, close_wait, NULL);
    }

}

static void in_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *msgCode = dict_find(iter, APP_KEY_MSG_CODE);
    const uint8_t msgType = msgCode->value->uint8;

    Tuple *message, *page, *totalPages, *data;
    message = dict_find(iter, APP_KEY_MESSAGE);
    page = dict_find(iter, APP_KEY_PAGE);
    data = dict_find(iter, APP_KEY_DATA);
    totalPages = dict_find(iter, APP_KEY_TOTAL_PAGES);

    switch (msgType) {
        case PB_MSG_IN_INIT_FAILED:
            handle_init_failed(message->value->cstring);
            break;
        case PB_MSG_IN_START_APP:
            handle_start_app();
            break;

        case PB_MSG_IN_THEATRES:
        case PB_MSG_IN_MOVIES:
        case PB_MSG_IN_MOVIE_THEATRES:
        case PB_MSG_IN_THEATRE_MOVIES:
        case PB_MSG_IN_SHOWTIMES:
            handle_data_received(msgType, page->value->uint8, totalPages->value->uint8, data->value->cstring);
            break;
        
        case PB_MSG_IN_CONNECTION_ERROR:
        case PB_MSG_IN_NO_DATA:
            if(preloader.statusText){
                text_layer_set_text(preloader.statusText, message->value->cstring);
                //@todo stop animation
            }
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

static void handle_init_failed(const char *message) {
    text_layer_set_text(splashScreen.statusText, message);
    splashScreen.loading = 0;
}

static void handle_start_app() {
    splashScreen.loading = 0;
    window_stack_pop(false);
    window_destroy(splashScreen.window);
    //init home screen
    home_screen_init();
}

char *strdup(const char *s) {
    char *d = malloc(strlen(s) + 1); // Allocate memory
    if (d != NULL) strcpy(d, s); // Copy string if okay
    return d; // Return new memory
}

char** str_split(char* a_str, const char a_delim) {
    char** result = 0;
    size_t count = 0;
    char* tmp = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof (char*) * count);

    if (result) {
        size_t idx = 0;
        char* token = strtok(a_str, delim);

        while (token) {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        //assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
