#ifndef TIMER_H
#define TIMER_H

#ifdef _WIN32
#include<profileapi.h>
#define TIME_T LARGE_INTEGER
#else
#define TIME_T unsigned long long
#endif

TIME_T current_time, last_frame_time, frequency;

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