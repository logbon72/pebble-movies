#pragma once

enum MovieUIMode {
    MovieUIModeMovies,
    MovieUIModeTheatreMovies,
};


#define MOVIE_FLD_LENGTH_ID 6
#define MOVIE_FLD_LENGTH_TITLE 25
#define MOVIE_FLD_LENGTH_GENRE 10
#define MOVIE_FLD_LENGTH_USER_RATING 6
#define MOVIE_FLD_LENGTH_CRITC_RATING 3
#define MOVIE_FLD_LENGTH_RUNTIME 4
#define MOVIE_FLD_LENGTH_RATED 6

//id,title,genre,user_rating,rated,critic_rating,runtime                    
#define MOVIE_FLD_IDX_ID 0
#define MOVIE_FLD_IDX_TITLE 1
#define MOVIE_FLD_IDX_GENRE 2
#define MOVIE_FLD_IDX_USER_RATING 3
#define MOVIE_FLD_IDX_RATED 4
#define MOVIE_FLD_IDX_CRITC_RATING 5
#define MOVIE_FLD_IDX_RUNTIME 6

struct MovieRecord {
    char id[MOVIE_FLD_LENGTH_ID];
    char title[MOVIE_FLD_LENGTH_TITLE];
    char genre[MOVIE_FLD_LENGTH_GENRE];
    char userRating[MOVIE_FLD_LENGTH_USER_RATING];
    char criticRating[MOVIE_FLD_LENGTH_CRITC_RATING];
    char runtime[MOVIE_FLD_LENGTH_RUNTIME];
    char rated[MOVIE_FLD_LENGTH_RATED];
} movie;


void movies_screen_init(int, enum MovieUIMode, char*);