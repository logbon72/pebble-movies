#pragma once
/* 
 * File:   preloader.h
 * Author: intelworx
 *
 * Created on March 19, 2014, 6:48 PM
 */

struct PreloaderScreen {
    Window *window;
    TextLayer *statusText;
    uint8_t isOn;
    AppTimer *timer;
} preloader;

void preloader_init(char* text);
