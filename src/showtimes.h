#pragma once

#define MAX_SHOWTIMES_COUNT 25

#define SHOWTIME_FLD_LENGTH_ID 9 
#define SHOWTIME_FLD_LENGTH_TYPE 2
#define SHOWTIME_FLD_LENGTH_TIME 8
#define SHOWTIME_FLD_LENGTH_LINK 2

#define SHOWTIME_FLD_IDX_ID 0
#define SHOWTIME_FLD_IDX_TYPE 1
#define SHOWTIME_FLD_IDX_TIME 2
#define SHOWTIME_FLD_IDX_LINK 3

struct Showtime {
    char id[SHOWTIME_FLD_LENGTH_ID];
    char type[SHOWTIME_FLD_LENGTH_TYPE];
    char time[SHOWTIME_FLD_LENGTH_TIME];
    char link[SHOWTIME_FLD_LENGTH_LINK];
};

struct Showtime showtimes[MAX_SHOWTIMES_COUNT];

struct ShowtimesUIScreen {
    Window* window;
    MenuLayer *menuLayer;
    GBitmap *canBuyIcon;
    GBitmap *cantBuyIcon;
    uint8_t total;
    
} showtimesUI;

char SHOWTIMES_LIST[1024];

void showtimes_init();