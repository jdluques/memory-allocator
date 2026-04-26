#ifndef INTERNAL_H
#define INTERNAL_H

#include <pthread.h>

extern void *heap_start;
extern void *heap_end;

extern pthread_mutex_t heap_mutex;

#endif
