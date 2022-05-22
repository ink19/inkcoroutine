#ifndef _INKCOROUTINE_H
#define _INKCOROUTINE_H

#include <pthread.h>
#include <ucontext.h>

typedef struct schedule_thread_t {
  pthread_t thread_list;
  ucontext_t base_context;
} schedule_thread_t;

typedef struct schedule_pipeline_t {
  int capacity;
} schedule_pipeline_t;

typedef struct schedule_context_t {
  ucontext_t base_context;
} schedule_context_t;

typedef struct schedule_t {
  schedule_thread_t **thread_list;
  int thread_number;
} schedule_t;



#endif