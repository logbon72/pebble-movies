#pragma once

/* 
 * File:   pbmovies.h
 * Author: intelworx
 *
 * Created on March 19, 2014, 1:50 AM
 */

/**
 * Buffer to hold showtimes
 */
char CurrentDateStr[17];
char *SHOWTIMES_BUFFER;
char *MOVIES_BUFFER;
char *THEATRES_BUFFER;

#define SHOWTIMES_BUFFER_MAX_SIZE 800
#define MOVIES_BUFFER_MAX_SIZE 2048
#define THEATRES_BUFFER_MAX_SIZE 1240

enum APP_KEYS {
    APP_KEY_MSG_CODE = 0x0, // TUPLE_INT (0,1)
    APP_KEY_MESSAGE = 0x1, // TUPLE_CSTRING
    APP_KEY_DATA = 0x2, // TUPLE_CSTRING
    APP_KEY_PAGE = 0x4, // TUPLE_CSTRING
    APP_KEY_SIZE = 0x8, // TUPLE_CSTRING
    APP_KEY_MOVIE_ID = 16, // TUPLE_CSTRING
    APP_KEY_THEATRE_ID = 32, // TUPLE_CSTRING
    APP_KEY_SHOWTIME_ID = 64, // TUPLE_CSTRING
    APP_KEY_DATE_OFFSET = 128,
    //APP_KEY_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

#define ARR_COUNT(x)  (sizeof(x) / sizeof(x[0]))
#define BUFFER_CREATE(size, maxSize)  ((char*) malloc(((size > maxSize ? maxSize : size)+1) * sizeof (char)))

//now define all message codes

enum PbMsgIn {
    PB_MSG_IN_INIT_FAILED = 0,
    PB_MSG_IN_START_APP,
    PB_MSG_IN_CONNECTION_ERROR,
    PB_MSG_IN_MOVIES,
    PB_MSG_IN_THEATRES,
    PB_MSG_IN_THEATRE_MOVIES,
    PB_MSG_IN_MOVIE_THEATRES,
    PB_MSG_IN_SHOWTIMES,
    PB_MSG_IN_NO_DATA,
    PB_MSG_IN_QR_CODE,
};

enum PbMsgOut {
    PB_MSG_OUT_INIT = 0,
    PB_MSG_OUT_GET_MOVIES = 1,
    PB_MSG_OUT_GET_THEATRES = 2,
    PB_MSG_OUT_GET_THEATRE_MOVIES = 3,
    PB_MSG_OUT_GET_MOVIE_THEATRES = 4,
    PB_MSG_OUT_GET_SHOWTIMES = 5,
    PB_MSG_OUT_GET_QR_CODE = 6,
};


#define PB_OUTBOX_SIZE 96
#define PB_INBOX_SIZE 2000
#define DELIMITER_FIELD '|'
#define DELIMITER_RECORD '\t'
#define MSG_INTERVAL_WAIT_MS 10000

void app_message_init(void);
short int record_count(char*, const char);
char *str_dup_range(char*, int, int, char *);
short int find_offset_of_nth_occurence(char*, char, char, int, short int);
char *get_data_at(char* data, int row, int col, char*, int);
int send_message_with_string(uint8_t, uint8_t, char *, uint8_t, char *);
void load_showtimes_for_movie_theatre();
void remove_top_window(int count);
