#ifndef UTIL_H
#define UTIL_H
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#ifdef _WIN32
    #ifdef DEBUG // Only need to include the full windows API in debug mode
        #define WIN32_LEAN_AND_MEAN
        #include<Windows.h>
        #include<fcntl.h>
    #endif
#include<profileapi.h>
#define TIME_T LARGE_INTEGER
#else
#define TIME_T unsigned long long
#endif

TIME_T current_time, last_frame_time, frequency;

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

// If we are on windows, this will open a terminal window and connect stdout to it
void open_console_window()
{
#if defined(_WIN32) && defined(DEBUG)
    FILE* io_file;
    long long console_handle, connection_handle;

    AllocConsole();
    SetConsoleTitle("Craftworlds Debug Console");
    console_handle = (long long)GetStdHandle(STD_OUTPUT_HANDLE);
    connection_handle = _open_osfhandle(console_handle, _O_TEXT);
    io_file = _fdopen(connection_handle, "w");
    *stdout = *io_file;

    console_handle = (long long)GetStdHandle(STD_INPUT_HANDLE);
    connection_handle = _open_osfhandle(console_handle, _O_TEXT);
    io_file = _fdopen(connection_handle, "w");
    *stdin = *io_file;

    console_handle = (long long)GetStdHandle(STD_ERROR_HANDLE);
    connection_handle = _open_osfhandle(console_handle, _O_TEXT);
    io_file = _fdopen(connection_handle, "w");
    *stderr = *io_file;
#endif
}

void initialise_timer()
{
    QueryPerformanceFrequency(&frequency);
    memset(&last_frame_time, 0, sizeof(TIME_T));
}

// Return the time over the last frame, in seconds
double get_elapsed_time()
{
    #ifdef _WIN32
    QueryPerformanceCounter(&current_time);
    double elapsed_time = ((double)(current_time.QuadPart - last_frame_time.QuadPart) / frequency.QuadPart);
    last_frame_time = current_time;
    #else
    current_time = SDL_GetPerformanceCounter();
    double elapsed_time = (((*current_time) - (*last_frame_time)) * 1000) / (*frequency);
    #endif
    return elapsed_time;
}

#endif