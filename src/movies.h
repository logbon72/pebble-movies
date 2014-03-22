#pragma once

enum MovieUIMode {
    MovieUIModeMovies,
    MovieUIModeTheatreMovies,
};

struct MovieUIScreen {
    Window *window;
    TextLayer *titleTxt;
    TextLayer *runtimeTxt;
    TextLayer *minTxt;
    TextLayer *ratedTxt;
    TextLayer *criticRatingLabelTxt;
    TextLayer *percentLabelTxt;
    TextLayer *criticRatingTxt;
    TextLayer *userRatingTxt;
    TextLayer *userRatingLabelTxt;
    TextLayer *genreTxt;
    ActionBarLayer *actionBar;

    GBitmap *downIcon;
    GBitmap *upIcon;
    GBitmap *selectIcon;
    uint8_t total;
    uint8_t currentIndex;
    char *currentTheatreId;
    enum MovieUIMode currentMode;
} moviesUI;
