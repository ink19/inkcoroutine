#ifndef _INKCOROUTINE_H
#define _INKCOROUTINE_H

#include <pthread.h>
#include <ucontext.h>
#include "list.h"

struct schedule_thread_t;

// 调度的上下文
typedef struct schedule_context_t {
  ucontext_t *base_context;
  struct schedule_thread_t *run_thread;
} schedule_context_t;

// 调度线程
typedef struct schedule_thread_t {
  pthread_t base_thread;
  schedule_context_t context;
  int continue_flag;
} schedule_thread_t;

typedef int (* schedule_pipeline_init_func_t)(void *srcp, const void *destp);
typedef int (* schedule_pipeline_destroy_func_t)(void *dp);

// 管道
typedef struct schedule_pipeline_t {
  ink_list_t list;
  int data_size;
  schedule_pipeline_init_func_t copy_func;
  schedule_pipeline_destroy_func_t destroy_func;
} schedule_pipeline_t;

// 调度
typedef struct schedule_t {
  schedule_thread_t **thread_list;
  int thread_number;
  pthread_mutex_t thread_mutex;

  schedule_pipeline_t **pipeline_list;
  int pipeline_number;

  schedule_context_t **context_list;
  int context_number;

  ink_list_t *wait_context_list;
} schedule_t;

extern schedule_t* schedule_init();

#endif
