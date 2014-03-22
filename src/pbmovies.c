#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"
#include "home.h"
#include "theatres.h"
#include "preloader.h"
#include "movies.h"
#include "showtimes.h"
#define MSG_CODE_NO_WAIT (uint8_t) 6000

static void handle_start_app(void);
static void handle_init_failed(char *message);
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

    //APP_LOG(APP_LOG_LEVEL_INFO, "Received data Length: %d, Page %d of %d", strlen(data), page, totalPages);
    if (page != 1 && page != totalPages && !(currentWaiter || currentWaiter != msgCode)) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Message discarded");
        //reset_mssage_receiver();
        return;
    }

    if (page != lastPage + 1) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Message broken");
        lastPage = 0;
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
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d has no Buffer", msgCode);
                return;
        }
    }

    while (*data != '\0') {
        *(messageBuffer++) = *data;
        ++data;
    }
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Concatenation successful, size now: %d", );


    if (inboxWaitTimer) {
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Canceling timer");
        app_timer_cancel(inboxWaitTimer);
        inboxWaitTimer = NULL;
    }

    if (page == totalPages) {
        *messageBuffer = '\0';


        reset_mssage_receiver();
        //preloader_set_is_on(0);
        preloader_stop();
        switch (msgCode) {

            case PB_MSG_IN_THEATRES:
                //APP_LOG(APP_LOG_LEVEL_INFO, "Records: (Length=%d) ", strlen(THEATRES_LIST));
                theatres_screen_initialize(record_count(THEATRES_LIST, DELIMITER_RECORD), TheatreUIModeTheatres, NULL);
                break;

            case PB_MSG_IN_MOVIE_THEATRES:
                //APP_LOG(APP_LOG_LEVEL_INFO, "Records: (Length=%d) ", strlen(THEATRES_LIST));
                theatres_screen_initialize(record_count(THEATRES_LIST, DELIMITER_RECORD), TheatreUIModeMovieThetares, currentMovie.id);
                break;

            case PB_MSG_IN_MOVIES:
                //APP_LOG(APP_LOG_LEVEL_INFO, "Movie Records: (Length=%d) ", strlen(MOVIES_LIST));
                //theatres_screen_initialize(record_count(THEATRES_LIST, DELIMITER_RECORD), TheatreUIModeTheatres, NULL);
                movies_screen_init(record_count(MOVIES_LIST, DELIMITER_RECORD), MovieUIModeMovies, NULL);
                break;

            case PB_MSG_IN_THEATRE_MOVIES:
                //APP_LOG(APP_LOG_LEVEL_INFO, "TheatreMovie Records: (Length=%d) ", strlen(MOVIES_LIST));
                movies_screen_init(record_count(MOVIES_LIST, DELIMITER_RECORD), MovieUIModeTheatreMovies, currentTheatre.id);
                break;

            case PB_MSG_IN_SHOWTIMES:
                showtimes_init();
                break;

            default:
                APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d needs no handling", msgCode);
        }
    } else {
        //some more messages expected, wait
        currentWaiter = msgCode;
        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting timer...");
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
            preloader_set_is_on(0);
            preloader_set_status(message->value->cstring);
            break;
        default:
            APP_LOG(APP_LOG_LEVEL_INFO, "Unknown message code %d", msgType);

    }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped! Reason: %d ", reason);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessageSendFailed! Reason : %d ", reason);
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

static void handle_init_failed(char *message) {
    splash_screen_set_loading(0);
    splash_screen_set_status_text(message);
}

static void handle_start_app() {
    splash_screen_hide();
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
    char *toReturn = str_dup_range(data, colStartOffset + 1, lenTmp, dest);
    toReturn[lenTmp] = '\0';
    return toReturn;
}

int send_message_with_string(uint8_t msgCode, uint8_t stringKey1, char *string1,
        uint8_t stringKey2, char *string2) {

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter == NULL) {
        return 0;
    }

    Tuplet msgTuplet = TupletInteger(APP_KEY_MSG_CODE, msgCode);
    dict_write_tuplet(iter, &msgTuplet);
    Tuplet strData = TupletCString(stringKey1, string1);
    dict_write_tuplet(iter, &strData);

    if (string2) {
        Tuplet strData2 = TupletCString(stringKey2, string2);
        dict_write_tuplet(iter, &strData2);
    }

    dict_write_end(iter);
    app_message_outbox_send();
    return 1;
}

void load_showtimes_for_movie_theatre() {
    if (currentMovie.id && currentTheatre.id) {
        int result = send_message_with_string(PB_MSG_OUT_GET_SHOWTIMES, APP_KEY_THEATRE_ID, currentTheatre.id
                , APP_KEY_MOVIE_ID, currentMovie.id);
        if (result) {
            preloader_init(LOADING_TEXT);
        }
    }
}