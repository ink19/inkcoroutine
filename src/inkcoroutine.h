#ifndef _INKCOROUTINE_H
#define _INKCOROUTINE_H

#include <pthread.h>
#include <ucontext.h>
#include "block_list.h"
#include "list.h"

#ifndef CVECTOR_LOGARITHMIC_GROWTH
#define CVECTOR_LOGARITHMIC_GROWTH
#endif

#include "c-vector/cvector.h"

struct ink_coroutine_thread_t;
struct ink_coroutine_running_t;
struct ink_coroutine_channel_t;
struct ink_coroutine_context_defer_callable_t;
struct ink_coroutine_t;

#define INK_COROUTINE_ALLOC(psize) (malloc(psize))
#define INK_COROUTINE_FREE(p) (free(p))

enum COROUTINE_CONTEXT_RUNNING_STATUS {
  COROUTINE_CONTEXT_RUNNING_STATUS_UNINIT = 0,
  COROUTINE_CONTEXT_RUNNING_STATUS_READY,
  COROUTINE_CONTEXT_RUNNING_STATUS_RUNNING,
  COROUTINE_CONTEXT_RUNNING_STATUS_WAIT, // 等待
  COROUTINE_CONTEXT_RUNNING_STATUS_FINISH
};

// 调度的上下文
typedef struct ink_coroutine_context_t {
  ucontext_t *base_context;
  struct ink_coroutine_running_t *running;
  void *base_stack;
  cvector_vector_type(struct ink_coroutine_context_defer_callable_t *) defer_list;

  enum COROUTINE_CONTEXT_RUNNING_STATUS running_status;
  struct ink_coroutine_t *sch;
} ink_coroutine_context_t;

enum COROUTINE_CONTEXT_THREAD_STATUS {
  COROUTINE_CONTEXT_THREAD_STATUS_UNINIT = 0,
  COROUTINE_CONTEXT_THREAD_STATUS_RUNNING,
  COROUTINE_CONTEXT_THREAD_STATUS_FINISH
};

// 调度线程
typedef struct ink_coroutine_thread_t {
  pthread_t base_thread;
  ink_coroutine_context_t *context;

  // 等待的管道
  // struct ink_coroutine_channel_t *wait_channel;
  enum COROUTINE_CONTEXT_THREAD_STATUS status;
} ink_coroutine_thread_t;

// 运行时保存的数据
typedef struct ink_coroutine_running_t {
  // 当前使用的线程
  ink_coroutine_thread_t *running_thread;
  // 当前运行上下文
  ink_coroutine_context_t *running_context;
  // 调度总的结构体
  struct ink_coroutine_t *sch;
} ink_coroutine_running_t;

// 协程函数类型
typedef void (* ink_coroutine_run_func_t)(ink_coroutine_running_t *running, void *args);

typedef struct ink_coroutine_context_defer_callable_t {
  ink_coroutine_run_func_t run_func;
  void *args;
} ink_coroutine_context_defer_callable_t;

enum COROUTINE_CHANNEL_STATUS {
  COROUTINE_CHANNEL_STATUS_UNINIT = 0,
  COROUTINE_CHANNEL_STATUS_ACTIVE,
  COROUTINE_CHANNEL_STATUS_FINISH
};

// 管道类型
typedef struct ink_coroutine_channel_t {
  int capacity;
  // 类型为 void *
  list_t* data;
  int size;

  // 等待该管道的列表 类型为ink_coroutine_context_t *
  list_t* read_wait;
  list_t* write_wait;

  enum COROUTINE_CHANNEL_STATUS status;
  pthread_mutex_t mutex;
} ink_coroutine_channel_t;

// 调度
typedef struct ink_coroutine_t {
  ink_coroutine_thread_t **thread_list;
  int thread_number;

  ink_coroutine_context_t **context_list;
  int context_number;

  // TODO 清理出一个WaitGroup库
  int finish_context_number;
  pthread_mutex_t finish_context_mutex;
  pthread_cond_t finish_context_cond;

  block_list_t *ready_context_list;
  pthread_cond_t wait_cond;
  pthread_mutex_t thread_mutex;
} ink_coroutine_t;

extern int ink_coroutine_init(ink_coroutine_t *sch, int thread_num);
extern int ink_coroutine_run(ink_coroutine_t *sch, ink_coroutine_run_func_t run_func, void *arg);
extern int ink_coroutine_join(ink_coroutine_t *sch);
extern int ink_coroutine_destroy(ink_coroutine_t *sch);
extern int ink_coroutine_context_defer(ink_coroutine_running_t *running, ink_coroutine_run_func_t run_func, void *arg);

// 管道相关
extern int ink_coroutine_channel_init(ink_coroutine_channel_t *chan, int max_capacity);
extern int ink_coroutine_channel_destroy(ink_coroutine_channel_t *chan);
extern void *ink_coroutine_channel_pop(ink_coroutine_running_t* running, ink_coroutine_channel_t* chan);
extern int ink_coroutine_channel_push(ink_coroutine_running_t* running, ink_coroutine_channel_t* chan, void *data);
extern int ink_coroutine_channel_close(ink_coroutine_running_t* running, ink_coroutine_channel_t* chan);

#endif
