#ifndef OS_H
#define OS_H
#include<stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#define HANDLE_TYPE HANDLE
#endif

void run_multithreaded(unsigned long (*function_to_run)(void*), void* inputs, size_t size_of_each_input, unsigned int num_inputs, unsigned int num_threads_to_use)
{
    unsigned int num_threads_in_use = 0;
    HANDLE_TYPE* thread_handles = calloc(num_threads_to_use, sizeof(HANDLE_TYPE));
    #ifdef _WIN32
    DWORD thread_id;
    for(unsigned int i = 0; i < num_inputs; i++)
    {
        thread_handles[num_threads_in_use++] = CreateThread(NULL, 0, function_to_run, (char*)inputs + (i * size_of_each_input), 0, &thread_id);
        if(num_threads_in_use == num_threads_to_use || i == num_inputs - 1)
        {
            WaitForMultipleObjects(num_threads_in_use, thread_handles, TRUE, INFINITE);
            num_threads_in_use = 0;
        }
    }
    #endif
    free(thread_handles);
}
#endif