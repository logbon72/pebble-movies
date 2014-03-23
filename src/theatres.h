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
//struct TheatreUI movieTheatresUI;

#define THEATRE_FLD_SIZE_ID 7
#define THEATRE_FLD_SIZE_NAME 33
#define THEATRE_FLD_SIZE_ADDR 33
#define THEATRE_FLD_SIZE_DISTANCE 9

struct TheatreRecord {
    char id[THEATRE_FLD_SIZE_ID];
    char name[THEATRE_FLD_SIZE_NAME];
    char address[THEATRE_FLD_SIZE_ADDR];
    char distance[THEATRE_FLD_SIZE_DISTANCE];
} currentTheatre;


char THEATRES_LIST[2500];

void theatres_screen_initialize(int, enum TheatreUiMode, char*);