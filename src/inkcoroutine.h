#ifndef _INKCOROUTINE_H
#define _INKCOROUTINE_H

#include <pthread.h>
#include <ucontext.h>
#include "tlist.h"
#include "list.h"

#ifndef CVECTOR_LOGARITHMIC_GROWTH
#define CVECTOR_LOGARITHMIC_GROWTH
#endif

#include "c-vector/cvector.h"

struct schedule_thread_t;
struct schedule_running_t;
struct schedule_channel_t;
struct schedule_t;

enum SCHEDULE_CONTEXT_RUNNING_STATUS {
  SCHEDULE_CONTEXT_RUNNING_STATUS_READY = 0,
  SCHEDULE_CONTEXT_RUNNING_STATUS_RUNNING,
  SCHEDULE_CONTEXT_RUNNING_STATUS_WAIT, // 等待
  SCHEDULE_CONTEXT_RUNNING_STATUS_FINISH
};

// 调度的上下文
typedef struct schedule_context_t {
  ucontext_t *base_context;
  struct schedule_running_t *running;

  enum SCHEDULE_CONTEXT_RUNNING_STATUS running_status;
  struct schedule_t *sch;
} schedule_context_t;

// 调度线程
typedef struct schedule_thread_t {
  pthread_t base_thread;
  schedule_context_t *context;

  // 等待的管道
  struct schedule_channel_t *wait_channel;
  int continue_flag;
} schedule_thread_t;

// 运行时保存的数据
typedef struct schedule_running_t {
  // 当前使用的线程
  schedule_thread_t *running_thread;
  // 当前运行上下文
  schedule_context_t *running_context;
  // 调度总的结构体
  struct schedule_t *sch;
} schedule_running_t;

// 协程函数类型
typedef void (* schedule_run_func_t)(schedule_running_t *running, void *args);

// 管道类型
typedef struct schedule_channel_t {
  int capacity;
  // 类型为 void *
  list_t* data;
  int size;

  // 等待该管道的列表 类型为schedule_context_t *
  list_t* read_wait;
  list_t* write_wait;

  pthread_mutex_t mutex;
} schedule_channel_t;

// 调度
typedef struct schedule_t {
  schedule_thread_t **thread_list;
  int thread_number;

  schedule_channel_t **pipeline_list;
  int pipeline_number;

  schedule_context_t **context_list;
  int context_number;
  int finish_context_number;
  pthread_mutex_t finish_context_mutex;
  pthread_cond_t finish_context_cond;

  ink_list_t *ready_context_list;
  pthread_cond_t wait_cond;
  pthread_mutex_t thread_mutex;
} schedule_t;

extern schedule_t* schedule_init(int thread_num);
extern schedule_channel_t *schedule_channel_init(int max_capacity);
extern void *schedule_channel_pop(schedule_running_t*running, schedule_channel_t* chan);
extern void schedule_channel_push(schedule_running_t*running, schedule_channel_t* chan, void *data);
extern void schedule_run(schedule_t *sch, schedule_run_func_t run_func, void *arg);
extern void schedule_join(schedule_t *sch);

#endif
