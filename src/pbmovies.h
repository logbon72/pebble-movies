#pragma once

/* 
 * File:   pbmovies.h
 * Author: intelworx
 *
 * Created on March 19, 2014, 1:50 AM
 */

enum APP_KEYS {
    APP_KEY_MSG_CODE = 0x0, // TUPLE_INT (0,1)
    APP_KEY_MESSAGE = 0x1, // TUPLE_CSTRING
    APP_KEY_CITY_KEY = 0x2, // TUPLE_CSTRING
    //APP_KEY_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

#define ARR_COUNT(x)  (sizeof(x) / sizeof(x[0]));

//now define all message codes

enum PbMsgIn {
    PB_MSG_IN_INIT_FAILED = 0,
    PB_MSG_IN_START_APP = 1,
};

enum PbMsgOut {
    PB_MSG_OUT_INIT = 0,
    PB_MSG_OUT_GET_MOVIES = 1,
    PB_MSG_OUT_GET_THEATRES = 2,
    PB_MSG_OUT_GET_THEATRE_MOVIES = 3,
    PB_MSG_OUT_GET_MOVIE_THEATRES = 4,
    PB_MSG_OUT_GET_SHOWTIMES = 5,
};


#define PB_OUTBOX_SIZE 64
#define PB_INBOX_SIZE 1024
const GBitmap *statusBarIcon;
void app_message_init(void);