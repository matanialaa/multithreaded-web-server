//
// Created by matan on 1/7/2023.
//

#include "myThread.h"
#include <pthread.h>
#include <stdlib.h>



struct thread_t{

    //pthread_t thread;
    int static_requests;
    int dynamic_requests;
    int total_requests;
};

my_thread my_thread_create()
{

    my_thread new_thread= (my_thread)malloc(sizeof(*new_thread));
   // int *thread_args = &index;
    //pthread_create(&(new_thread->thread), NULL,thread_func , (void *)thread_args);
    new_thread->static_requests=0;
    new_thread->dynamic_requests=0;
    new_thread->total_requests=0;

    return new_thread ;
}

void my_thread_destroy(my_thread new_thread)
{
    ///if we figured out that we will do join for the threads then we need to destroy the struct.
    //free(new_thread);
}

int my_thread_get_static_requests_num(my_thread new_thread)
{
    return new_thread->static_requests;
}

int my_thread_get_dynamic_requests_num(my_thread new_thread)
{
    return new_thread->dynamic_requests;
}
int my_thread_get_total_requests_num(my_thread new_thread)
{
    return new_thread->total_requests;
}
void my_thread_increase_static_requests_num(my_thread new_thread)
{
    new_thread->static_requests++;
}
void my_thread_increase_dynamic_requests_num(my_thread new_thread)
{
    new_thread->dynamic_requests++;
}
void my_thread_increase_total_requests_num(my_thread new_thread)
{
    new_thread->total_requests++;
}