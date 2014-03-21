#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"
#include "home.h"
#include "theatres.h"
#include "preloader.h"
#define MSG_CODE_NO_WAIT (uint8_t) 6000

static void handle_start_app(void);
static void handle_init_failed(const char *message);
static void handle_data_received(uint8_t, uint8_t, uint8_t, char *);
static void close_wait(void *);

char *messageBuffer;
AppTimer *inboxWaitTimer;
uint8_t currentWaiter = MSG_CODE_NO_WAIT;
char **dataRecords;

static void close_wait(void *context) {
    if (currentWaiter == MSG_CODE_NO_WAIT) {
        handle_data_received(currentWaiter, 0, 0, "");
    }

    if (inboxWaitTimer) {
        app_timer_cancel(inboxWaitTimer);
    }
}

static void handle_data_received(uint8_t msgCode, uint8_t page, uint8_t totalPages, char *data) {

    APP_LOG(APP_LOG_LEVEL_INFO, "Received data Length: %d, Page %d of %d", strlen(data), page, totalPages);
    if (page != 1 && page != totalPages && !(currentWaiter || currentWaiter != msgCode)) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Message discarded, wait is over already");
        return;
    }

    if (!messageBuffer) {
        messageBuffer = strdup(data);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "message copied to buffer");
    } else {
        char *tmp = strdup(messageBuffer);
        free(messageBuffer);
        size_t newLen = strlen(tmp) + strlen(data) + 1;
        messageBuffer = malloc(sizeof (char*) * newLen);
        strcat(messageBuffer, tmp);
        strcat(messageBuffer, data);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Concatenation successful, size now: %d", strlen(messageBuffer));
        free(tmp);
    }

    //    if(data){
    //        free(data);
    //    }


    if (inboxWaitTimer) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Canceling timer");
        app_timer_cancel(inboxWaitTimer);
    }

    if (page == totalPages) {
        currentWaiter = MSG_CODE_NO_WAIT;
        //char **dataRecords = str_split(messageBuffer, DELIMITER_RECORD);
        int length = 0;
        dataRecords = str_split(messageBuffer, DELIMITER_RECORD, &length);
        free(messageBuffer);
        messageBuffer = NULL;

        APP_LOG(APP_LOG_LEVEL_INFO, "Records: (Length=%d) ", length);
        //        for(int i=0; i < length;i++){
        //            APP_LOG(APP_LOG_LEVEL_DEBUG, "Record %d : %s", i+1, dataRecords[i]);
        //        }
        window_stack_pop(false);
        switch (msgCode) {

            case PB_MSG_IN_THEATRES:
                theatresUI.theatres = dataRecords;
                theatres_screen_initialize(length, THEATRE_UI_MODE_THEATRES, NULL);
                break;

            default:
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d needs no handling", msgCode);
        }

    } else {
        //some more messages expected, wait
        currentWaiter = msgCode;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting timer...");
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
            if (preloader.statusText) {
                text_layer_set_text(preloader.statusText, message->value->cstring);
                //@todo stop animation
            }
            break;
        default:
            APP_LOG(APP_LOG_LEVEL_INFO, "Unknown message code received %d", msgType);

    }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped! Reason: %d ", reason);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send! Reason : %d ", reason);
}

void app_message_init() {
    // Register message handlers
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    // Init buffers
    app_message_open(PB_INBOX_SIZE, PB_OUTBOX_SIZE);
    //fetch_msg();
    //uint32_t needed= dict_calc_buffer_size(3, sizeof (uint8_t), sizeof (uint8_t), 500 * sizeof (char*));
    //APP_LOG(APP_LOG_LEVEL_INFO, "Needed Inbox %lu, available: %lu", needed, app_message_inbox_size_maximum());
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

char** str_split(char *src_str, const char delimeter, int *length) {
    //replace deliminator's with zeros and count how many
    //sub strings with length >= 1 exist
    int num_sub_str = 1;
    char *loopTruStr = src_str;
    short int found_delim = false;
    while (*loopTruStr) {
        if (*loopTruStr == delimeter) {
            *loopTruStr = 0;
            found_delim = true;
        } else if (found_delim) { //found first character of a new string
            num_sub_str++;
            found_delim = false;
            //sub_str_vec.push_back(src_str_tmp); //for c++
        }
        loopTruStr++;
    }
    //printf("Start - found %d sub strings\n", num_sub_str);
    if (num_sub_str <= 1) {
        return (0);
    }

    //if you want to use a c++ vector and push onto it, the rest of this function
    //can be omitted (obviously modifying input parameters to take a vector, etc)

    char **sub_strings = (char **) malloc((sizeof (char*) * num_sub_str) + 1);
    const char *src_str_terminator = loopTruStr;
    loopTruStr = src_str;
    short int found_null = true;
    int idx = 0;
    while (loopTruStr < src_str_terminator) {
        if (!*loopTruStr) //found a NULL
            found_null = true;
        else if (found_null) {
            sub_strings[idx++] = loopTruStr;
            //printf("sub_string_%d: [%s]\n", idx-1, sub_strings[idx-1]);
            found_null = false;
        }
        loopTruStr++;
    }
    sub_strings[num_sub_str] = NULL;
    if (length) {
        *length = num_sub_str;
    }

    return (sub_strings);
}