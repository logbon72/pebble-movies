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

//static const char *blankStr = "";
char *messageBuffer;
AppTimer *inboxWaitTimer;
uint8_t currentWaiter = MSG_CODE_NO_WAIT;
//char **dataRecords;
uint8_t lastPage = 0;

static void close_wait(void *context) {
    if (currentWaiter == MSG_CODE_NO_WAIT) {
        handle_data_received(currentWaiter, 0, 0, "");
    }

    if (inboxWaitTimer) {
        app_timer_cancel(inboxWaitTimer);
    }
}

static void reset_mssage_receiver() {
    messageBuffer = NULL;
    lastPage = 0;
    if (inboxWaitTimer) {
        app_timer_cancel(inboxWaitTimer);
    }
    currentWaiter = MSG_CODE_NO_WAIT;
}

static void handle_data_received(uint8_t msgCode, uint8_t page, uint8_t totalPages, char *data) {

    APP_LOG(APP_LOG_LEVEL_INFO, "Received data Length: %d, Page %d of %d", strlen(data), page, totalPages);
    if (page != 1 && page != totalPages && !(currentWaiter || currentWaiter != msgCode)) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Message discarded, wait is over already");
        return;
    }

    if (page != lastPage + 1) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Message broken");
        return;
    }

    lastPage = page;


    if (page == 1) {
        //buffer is should be assigned
        switch (msgCode) {

            case PB_MSG_IN_THEATRES:
            case PB_MSG_IN_MOVIE_THEATRES:
                messageBuffer = THEATRES_LIST;
                break;
            case PB_MSG_IN_MOVIES:
            case PB_MSG_IN_THEATRE_MOVIES:
                messageBuffer = MOVIES_LIST;
                break;

            case PB_MSG_IN_SHOWTIMES:
                messageBuffer = SHOWTIMES_LIST;
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d has no Buffer", msgCode);
                return;
        }
    }

    while (*data != '\0') {
        *(messageBuffer++) = *data;
        ++data;
    }
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Concatenation successful, size now: %d", );


    if (inboxWaitTimer) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Canceling timer");
        app_timer_cancel(inboxWaitTimer);
        inboxWaitTimer = NULL;
    }

    if (page == totalPages) {
        *messageBuffer = '\0';
        window_stack_pop(false);
        reset_mssage_receiver();
        switch (msgCode) {

            case PB_MSG_IN_THEATRES:
                APP_LOG(APP_LOG_LEVEL_INFO, "Records: (Length=%d) ", strlen(THEATRES_LIST));
                theatres_screen_initialize(record_count(THEATRES_LIST, DELIMITER_RECORD), TheatreUIModeTheatres, NULL);
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
            preloader.isOn = 0;
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

short int record_count(char* string, const char delimeter) {
    int c = 0, count = 0;
    while (string[c] != '\0') {
        if (string[c] == delimeter) {
            count++;
        }
        c++;
    }

    return count + 1;
}

char *str_dup_range(char* input, int offset, int len, char *dest) {
    int input_len = strlen(input);
    if (offset + len > input_len) {
        return NULL;
    }

    //char *dest = malloc(sizeof (char*) * len);
    strncpy(dest, input + offset, len);

    return dest;
}

short int find_offset_of_nth_occurence(char* inString, char forChar, char terminator, int n, short int startFrom) {
    if (n <= 0) {
        return startFrom;
    }
    short int offset, totalFound = 0;
    for (offset = startFrom; (inString[offset] != terminator && inString[offset] != '\0'); offset++) {
        if (inString[offset] == forChar) {
            if (++totalFound == n) {
                return offset;
            }
        }
    }

    return -1;
}

char *get_data_at(char* data, int row, int col, char dest[], int maxLength) {
    int rowOffset = row <= 0 ? -1 : find_offset_of_nth_occurence(data, DELIMITER_RECORD, '\0', row, 0);
    //get starting point of the row
    //starting point of column, start reading from row
    int colStartOffset = col <= 0 ? rowOffset : find_offset_of_nth_occurence(data, DELIMITER_FIELD, DELIMITER_RECORD, col, rowOffset + 1);

    for (uint16_t i = 0; i < maxLength - 1; i++) {
        dest[i] = ' ';
    }
    dest[maxLength - 1] = '\0';
    //APP_LOG(APP_LOG_LEVEL_INFO, "Length : %d", maxLength);
    //if none of such, then return NULL
    //strncpy(dest, blankStr, 1);
    if (colStartOffset < 0 && col > 0) {
        return dest;
    }

    //get ending of column
    int colEndOffset = find_offset_of_nth_occurence(data, DELIMITER_FIELD, DELIMITER_RECORD, 1, colStartOffset + 1);
    //not found? then probably record end
    if (colEndOffset < 0) {
        colEndOffset = find_offset_of_nth_occurence(data, DELIMITER_RECORD, '\0', 1, colStartOffset + 1);
        //still not found ? then probably data end.
        if (colEndOffset < 0) {
            colEndOffset = strlen(data);
        }
    }

    int lenTmp = colEndOffset - colStartOffset - 1;
    if (lenTmp > (maxLength - 1)) {
        lenTmp = maxLength - 1;
    }
    //APP_LOG(APP_LOG_LEVEL_INFO, "Offset %d to %d - Length = %d", colStartOffset, colEndOffset, lenTmp);
    return str_dup_range(data, colStartOffset + 1, lenTmp, dest);
}