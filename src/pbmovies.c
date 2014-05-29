#include <pebble.h>
#include "pbmovies.h"
#include "splash_screen.h"
#include "home.h"
#include "theatres.h"
#include "preloader.h"
#include "movies.h"
#include "showtimes.h"
#include "qrcode.h"
#include "start.h"

#define MSG_CODE_NO_WAIT 0xff
#define JS_DATA_PER_SEND 240

static void handle_start_app(void);
static void handle_init_failed(char *message);
static void handle_data_received(uint8_t, uint8_t, uint8_t, Tuple *);
static void close_wait(void *);

static char *messageBuffer;
static uint8_t *bytesBuffer;
AppTimer *inboxWaitTimer;
uint8_t lastPage = 0;
static int totalDataReceived = 0;

//static void close_wait(void *context) {
//    if (currentWaiter == MSG_CODE_NO_WAIT) {
//        handle_data_received(currentWaiter, 0, 0, NULL);
//    }
//
//    if (inboxWaitTimer) {
//        app_timer_cancel(inboxWaitTimer);
//    }
//}

static void reset_message_receiver() {
    messageBuffer = NULL;
    bytesBuffer = NULL;
    totalDataReceived = lastPage = 0;
    if (inboxWaitTimer) {
        app_timer_cancel(inboxWaitTimer);
    }
    //currentWaiter = MSG_CODE_NO_WAIT;
}

static void handle_data_received(uint8_t msgCode, uint8_t page,
        uint8_t totalPages, Tuple *tuple) {

    //APP_LOG(APP_LOG_LEVEL_INFO, "Received data Length: %d, Page %d of %d", dataLength, page, totalPages);
    //    if (page != 1 && page != totalPages && !(currentWaiter || currentWaiter != msgCode)) {
    //        //APP_LOG(APP_LOG_LEVEL_INFO, "Message discarded");
    //        //reset_mssage_receiver();
    //        return;
    //    }

    if (!tuple || page != lastPage + 1) {
        //APP_LOG(APP_LOG_LEVEL_WARNING, "Message broken");
        lastPage = 0;
        totalDataReceived = 0;
        messageBuffer = NULL;
        preloader_set_timed_out();
        return;
    }

    lastPage = page;
    uint8_t stringDataMode = msgCode != PB_MSG_IN_QR_CODE;
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

            case PB_MSG_IN_QR_CODE:
                bytesBuffer = QR_CODE_BUFFER;
                break;

            default:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d has no Buffer", msgCode);
                return;
        }
    }


    //if terminating with null, then fail
    if (stringDataMode) {
        char *data = tuple->value->cstring;
        while (!*data == '\0') {
            *(messageBuffer++) = *data;
            ++data;
            ++totalDataReceived;
        }
    } else {
        //memcpy(ctx->data + ctx->index, tuple->value->data, tuple->length);
        //APP_LOG(APP_LOG_LEVEL_DEBUG,"Data length: %d", tuple->length);
        memcpy(QR_CODE_BUFFER + (page - 1) * JS_DATA_PER_SEND, tuple->value->data, tuple->length);
        totalDataReceived += tuple->length;
    }


    //    if (inboxWaitTimer) {
    //        app_timer_cancel(inboxWaitTimer);
    //        inboxWaitTimer = NULL;
    //    }
    //free(tuple->value->data);
    //free(tuple->value);


    if (page == totalPages) {
        if (stringDataMode && messageBuffer) {
            *messageBuffer = '\0';
        }

        //APP_LOG(APP_LOG_LEVEL_DEBUG,"Total Received %d",totalDataReceived);
        reset_message_receiver();
        //preloader_set_is_on(0);
        preloader_stop();
        switch (msgCode) {

            case PB_MSG_IN_THEATRES:
                //APP_LOG(APP_LOG_LEVEL_INFO, "Records: (Length=%d) ", strlen(THEATRES_LIST));
                theatres_screen_initialize(record_count(THEATRES_LIST, DELIMITER_RECORD), TheatreUIModeTheatres, NULL);
                break;

            case PB_MSG_IN_MOVIE_THEATRES:
                //APP_LOG(APP_LOG_LEVEL_INFO, "Records: (Length=%d) ", strlen(THEATRES_LIST));
                theatres_screen_initialize(record_count(THEATRES_LIST, DELIMITER_RECORD), TheatreUIModeMovieThetares, movie.id);
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

            case PB_MSG_IN_QR_CODE:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "Data received, length, %d", strlen(QR_CODE_BUFFER));
                qr_code_init();
                break;

                //default:
                //APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Code %d needs no handling", msgCode);
        }
    }
    //  else {
    //        //some more messages expected, wait
    //        currentWaiter = msgCode;
    //        //APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting timer...");
    //        inboxWaitTimer = app_timer_register(MSG_INTERVAL_WAIT_MS, close_wait, NULL);
    //    }

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
        case PB_MSG_IN_QR_CODE:
            handle_data_received(msgType, page->value->uint8, totalPages->value->uint8,
                    data);
            break;

        case PB_MSG_IN_CONNECTION_ERROR:
        case PB_MSG_IN_NO_DATA:
            preloader_set_is_on(0);
            preloader_set_status(message->value->cstring);
            break;
            //        default:
            //            APP_LOG(APP_LOG_LEVEL_INFO, "Unknown message code %d", msgType);

    }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MsgDropped_%d", reason);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "MsgSendFailed_%d", reason);
}

void app_message_init() {
    // Register message handlers
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_failed(out_failed_handler);
    //Init buffers
    app_message_open(PB_INBOX_SIZE, PB_OUTBOX_SIZE);
    //fetch_msg();
    //    uint32_t needed= dict_calc_buffer_size(3, sizeof (uint8_t), sizeof (uint8_t), 128 * sizeof (char*));
    //    APP_LOG(APP_LOG_LEVEL_INFO, "Needed Inbox %lu, available: %lu", needed, app_message_inbox_size_maximum());
}

static void handle_init_failed(char *message) {
    splash_screen_set_loading(0);
    splash_screen_set_status_text(message);
}

static void handle_start_app() {
    splash_screen_hide();
    //home_screen_init();
    start_screen_init();
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

    if(!bluetooth_connection_service_peek()){
        preloader_init();
        preloader_set_no_connect();
        return 0;
    }
    
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
    if (movie.id && currentTheatre.id) {
        int result = send_message_with_string(PB_MSG_OUT_GET_SHOWTIMES, APP_KEY_THEATRE_ID, currentTheatre.id
                , APP_KEY_MOVIE_ID, movie.id);
        if (result) {
            preloader_init();
        }
    }
}