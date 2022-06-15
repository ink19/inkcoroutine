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
struct schedule_context_defer_callable_t;
struct schedule_t;

#define INK_SCHEDULE_ALLOC(psize) (malloc(psize))
#define INK_SCHEDULE_FREE(p) (free(p))

enum SCHEDULE_CONTEXT_RUNNING_STATUS {
  SCHEDULE_CONTEXT_RUNNING_STATUS_UNINIT = 0,
  SCHEDULE_CONTEXT_RUNNING_STATUS_READY,
  SCHEDULE_CONTEXT_RUNNING_STATUS_RUNNING,
  SCHEDULE_CONTEXT_RUNNING_STATUS_WAIT, // 等待
  SCHEDULE_CONTEXT_RUNNING_STATUS_FINISH
};

// 调度的上下文
typedef struct schedule_context_t {
  ucontext_t *base_context;
  struct schedule_running_t *running;
  void *base_stack;
  cvector_vector_type(struct schedule_context_defer_callable_t *) defer_list;

  enum SCHEDULE_CONTEXT_RUNNING_STATUS running_status;
  struct schedule_t *sch;
} schedule_context_t;

enum SCHEDULE_CONTEXT_THREAD_STATUS {
  SCHEDULE_CONTEXT_THREAD_STATUS_UNINIT = 0,
  SCHEDULE_CONTEXT_THREAD_STATUS_RUNNING,
  SCHEDULE_CONTEXT_THREAD_STATUS_FINISH
};

// 调度线程
typedef struct schedule_thread_t {
  pthread_t base_thread;
  schedule_context_t *context;

  // 等待的管道
  // struct schedule_channel_t *wait_channel;
  enum SCHEDULE_CONTEXT_THREAD_STATUS status;
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

typedef struct schedule_context_defer_callable_t {
  schedule_run_func_t run_func;
  void *args;
} schedule_context_defer_callable_t;

enum SCHEDULE_CHANNEL_STATUS {
  SCHEDULE_CHANNEL_STATUS_UNINIT = 0,
  SCHEDULE_CHANNEL_STATUS_ACTIVE,
  SCHEDULE_CHANNEL_STATUS_FINISH
};

// 管道类型
typedef struct schedule_channel_t {
  int capacity;
  // 类型为 void *
  list_t* data;
  int size;

  // 等待该管道的列表 类型为schedule_context_t *
  list_t* read_wait;
  list_t* write_wait;

  enum SCHEDULE_CHANNEL_STATUS status;
  pthread_mutex_t mutex;
} schedule_channel_t;

// 调度
typedef struct schedule_t {
  schedule_thread_t **thread_list;
  int thread_number;

  schedule_context_t **context_list;
  int context_number;

  // TODO 清理出一个WaitGroup库
  int finish_context_number;
  pthread_mutex_t finish_context_mutex;
  pthread_cond_t finish_context_cond;

  ink_list_t *ready_context_list;
  pthread_cond_t wait_cond;
  pthread_mutex_t thread_mutex;
} schedule_t;

extern int schedule_init(schedule_t *sch, int thread_num);
extern int schedule_run(schedule_t *sch, schedule_run_func_t run_func, void *arg);
extern int schedule_join(schedule_t *sch);
extern int schedule_destroy(schedule_t *sch);
extern int schedule_context_defer(schedule_running_t *running, schedule_run_func_t run_func, void *arg);

// 管道相关
extern int schedule_channel_init(schedule_channel_t *chan, int max_capacity);
extern int schedule_channel_destroy(schedule_channel_t *chan);
extern void *schedule_channel_pop(schedule_running_t* running, schedule_channel_t* chan);
extern int schedule_channel_push(schedule_running_t* running, schedule_channel_t* chan, void *data);
extern int schedule_channel_close(schedule_running_t* running, schedule_channel_t* chan);

#endif
