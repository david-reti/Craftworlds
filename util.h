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
#endif