#include "segel.h"
#include "request.h"
#include "queue.h"
#include <pthread.h>
#include <stdlib.h>
#include "myThread.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


pthread_mutex_t mutex_lock;
pthread_cond_t cond;
pthread_cond_t block_mode_cond;
pthread_cond_t bf_mode_cond;

Queue waiting_req_queue = NULL;
Queue working_req_queue = NULL;

my_thread* threads_statistics=NULL;


// HW3: Parse the new arguments too
/*void getargs(int *port,int* threads_num, int* queue_max_size, char* schedalg, int argc, char *argv[])
{
    if (argc < 5) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }

    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_max_size = atoi(argv[3]);
    strcpy(schedalg, argv[4]);

}*/
void getargs(int *port, int *threads_num, int *queue_max_size, char *schedalg,int *dynamic_queue_max_size, int argc, char *argv[])
{
    if (argc < 5 || argc > 6) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_max_size = atoi(argv[3]);
    strcpy(schedalg, argv[4]);

    if (strcmp(schedalg, "dynamic") == 0) {
        if(argc < 6){
            fprintf(stderr, "Usage: %s <port>\n", argv[0]);
            exit(1);
        }
        *dynamic_queue_max_size = atoi(argv[5]);
    }
}


void* thread_func(void* args) {

    int thread_index =*((int *)args);
    while(1){

        pthread_mutex_lock(&mutex_lock);
        while(queue_empty(waiting_req_queue)){
            pthread_cond_wait(&cond, &mutex_lock);
        }
        TimeVal arrival_time = queue_head_arrival_time(waiting_req_queue);
        //int curr_fd = queue_dequeue(waiting_req_queue);
        int curr_fd= queue_trans_between_2_queues(waiting_req_queue,working_req_queue,arrival_time);
       // queue_enqueue(working_req_queue, curr_fd,arrival_time);
        pthread_mutex_unlock(&mutex_lock);


        requestHandle(curr_fd, arrival_time,  threads_statistics, thread_index);
        Close(curr_fd);


        pthread_mutex_lock(&mutex_lock);
        //queue_dequeue_by_index(working_req_queue, queue_find(working_req_queue,curr_fd));
        pthread_cond_signal(&block_mode_cond);
        if(queue_size(working_req_queue) ==0 && queue_size(waiting_req_queue) == 0)
            pthread_cond_signal(&bf_mode_cond);
        pthread_mutex_unlock(&mutex_lock);

    }
}

int main(int argc, char *argv[])
{
    fprintf(stdout,"u are in the main thread");
    int listenfd, connfd, port, clientlen ,threads_num, queue_max_size,dynamic_queue_max_size;
    struct sockaddr_in clientaddr;
    char schedalg[8];
    ///should check malloc
    getargs(&port, &threads_num, &queue_max_size, schedalg, &dynamic_queue_max_size,argc, argv);


    // HW3: Create some threads...
    //queues create
    waiting_req_queue = queue_create(queue_max_size);
    working_req_queue = queue_create(threads_num);

    //mutex create
    pthread_mutex_init(&mutex_lock, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&block_mode_cond, NULL);
    pthread_cond_init(&bf_mode_cond, NULL);


    //threads create & threads statistics create
    threads_statistics = (my_thread*)malloc(sizeof(my_thread)*threads_num);
    pthread_t* threads = malloc(sizeof(*threads)*threads_num);
    //int * thread_args = (int*)malloc(sizeof(*thread_args));
    for(int i = 0; i< threads_num; i++){
        int index=i;
        int* thread_args= &index;
        //int thread_args[1] = {i,};

        pthread_create(&threads[i], NULL, thread_func, (void *)thread_args);
        threads_statistics[i]=my_thread_create();

    }

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);


	//
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a  and have one of the worker threads
	// do the work.
	//



    // if there is no enough buffers
        pthread_mutex_lock(&mutex_lock);
        struct timeval arrival_time;
        gettimeofday(&arrival_time,NULL);

        if(queue_size(working_req_queue) + queue_size(waiting_req_queue) == queue_max_size) {
            if(strcmp(schedalg, "block") == 0){
                while(queue_size(working_req_queue) + queue_size(waiting_req_queue) == queue_max_size){
                    pthread_cond_wait(&block_mode_cond, &mutex_lock);
                }
            }else if(strcmp("dt", schedalg)==0){
                fprintf(stdout,"u are in drop tail");
                Close(connfd);
                pthread_mutex_unlock(&mutex_lock);
                continue;
            }
            else if(strcmp("dh", schedalg)==0){
                //we should dequeue from the waiting queue the oldest request
                int old_request_fd = queue_dequeue(waiting_req_queue);
                if( old_request_fd== -1){
                    //we should drop the new request
                    Close(connfd);
                    pthread_mutex_unlock(&mutex_lock);
                    continue;
                }else{
                    Close(old_request_fd);
                    //it will keep going till reaching the enqueue of new request
                }
            }
            else if(strcmp("bf",schedalg)==0)
            {
                // Block_flush policy: Wait until queue is empty and no threads are handling requests
                while (queue_size(working_req_queue) > 0 || queue_size(waiting_req_queue) > 0) {
                    pthread_cond_wait(&bf_mode_cond, &mutex_lock);
                }
                //we should drop the new request
                Close(connfd);
                pthread_mutex_unlock(&mutex_lock);
                continue;
            }
            else if(strcmp("dynamic",schedalg)==0)
            {
                if(queue_max_size < dynamic_queue_max_size) {
                    fprintf(stdout,"increasing the queue size");
                    queue_max_size++;
                }

                //we should drop the new request
                fprintf(stdout, "dropping the new request");
                Close(connfd);
                pthread_mutex_unlock(&mutex_lock);
                continue;

            }
            else if(strcmp("random", schedalg)==0){
                if(queue_empty(waiting_req_queue)){
                    Close(connfd);
                    pthread_mutex_unlock(&mutex_lock);
                    continue;
                }
                else{
                    int half = (int)(queue_size(waiting_req_queue) + 1) / 2;
                    for (int i = 0; i < half; i++) {
                        ///added this
                        if(queue_empty(waiting_req_queue))
                            break;
                        Close(queue_dequeue_by_index(waiting_req_queue, (int) rand() % queue_size(waiting_req_queue)));
                    }
                }
            }
        }
    //if there is enough buffers
    //enter the request to the waiting queue
        queue_enqueue(waiting_req_queue, connfd, arrival_time);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex_lock);
    }

}


    


 
