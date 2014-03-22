#pragma once
/* 
 * File:   preloader.h
 * Author: intelworx
 *
 * Created on March 19, 2014, 6:48 PM
 */

void preloader_init(const char* );

void preloader_set_status(char *);
void preloader_set_is_on(uint8_t);
void preloader_stop();
void preloader_set_hidden(Window *window);