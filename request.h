#ifndef __REQUEST_H__
#include "myThread.h"
void requestHandle(int fd, struct timeval arrival_time, my_thread* threads, int thread_index);

#endif
