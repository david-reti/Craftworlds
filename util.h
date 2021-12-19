#ifndef UTIL_H
#define UTIL_H
#include<stdio.h>
#include<stdlib.h>
bool failed(int result) { return result < 0; }

void exit_with_error(const char* message, const char* detail)
{
    fprintf(stderr, "%s: %s\n", message, detail);
    exit(1);
}

unsigned int pressed_keys[1] = { 0 };
bool key_pressed(unsigned int key) { 
    for(unsigned char i = 0; i < 1; i++)
    {
        if(pressed_keys[i] == key) 
            return true;
    }
    return false;
}

void press_key(unsigned int key) {
    if(key_pressed(key)) return;  
    for(unsigned char i = 0; i < 1; i++)
    {
        if(pressed_keys[i] == 0) 
            pressed_keys[i] = key; 
    }
}

void release_key(unsigned int key) {  
    for(unsigned char i = 0; i < 1; i++)
    {
        if(pressed_keys[i] == key) 
            pressed_keys[i] = 0; 
    }
}

unsigned int clamp(unsigned int val, unsigned int lowend, unsigned int highend) { return val < lowend ? lowend : (val > highend ? highend : val); }
#endif