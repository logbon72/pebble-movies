#pragma once
/* 
 * File:   theatres.h
 * Author: intelworx
 *
 * Created on March 20, 2014, 11:54 AM
 */

enum TheatreUiMode {
    TheatreUIModeTheatres,
    TheatreUIModeMovieThetares,
};

struct TheatreUI {
    Window *window;
    ActionBarLayer *actionBar;
    //TextLayer *titleBar;
    TextLayer *address;
    TextLayer *name;
    TextLayer *distance;
    enum TheatreUiMode currentMode;
    //char **theatres;
    uint16_t currentIndex;
    uint16_t total;
    //char **current;
    char *currentMovie;
    GBitmap *upIcon;
    GBitmap *downIcon;
    GBitmap *selectIcon;
};

struct TheatreUI theatresUI;
//struct TheatreUI movieTheatresUI;

void theatres_screen_initialize(int, enum TheatreUiMode, char*);