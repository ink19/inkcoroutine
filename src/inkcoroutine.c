#include "inkcoroutine.h"

#include <stdio.h>
#include <stdlib.h>

#define LIST_MAX_SIZE 1024
#define STACK_SIZE (10 * 1024 * 1024)

// 切换上下文
int ink_swap_context(ink_coroutine_context_t* save_context, const ink_coroutine_context_t* to_context) {
  return swapcontext((save_context->base_context), (to_context->base_context));
}

// 切换出，等待执行程序
int ink_swap_out_context(ink_coroutine_context_t *save_context, ink_coroutine_thread_t* thread_obj) {
  save_context->running->running_thread = NULL;
  save_context->running_status = COROUTINE_CONTEXT_RUNNING_STATUS_WAIT;
  ink_swap_context(save_context, thread_obj->context);
  return 0;
}

int ink_context_destroy(ink_coroutine_context_t *context) {
  INK_COROUTINE_FREE(context->base_context);

  if (context->base_stack != NULL) {
    INK_COROUTINE_FREE(context->base_stack);
  }

  if (context->running != NULL) {
    INK_COROUTINE_FREE(context->running);
  }

  // 删除defer list 
  int defer_list_len = cvector_size(context->defer_list);
  for (int loop_i = 0; loop_i < defer_list_len; ++loop_i) {
    free(context->defer_list[loop_i]);
  }
  cvector_free(context->defer_list);

  return 0;
}

// 切换入，开始执行程序
int ink_swap_in_context(ink_coroutine_context_t *save_context, ink_coroutine_thread_t* thread_obj) {
  save_context->running->running_thread = thread_obj;
  save_context->running_status = COROUTINE_CONTEXT_RUNNING_STATUS_RUNNING;
  ink_swap_context(thread_obj->context, save_context);
  return 0;
}

// 完成程序时的收尾工作
int ink_context_finish(ink_coroutine_context_t *context) {
  context->running_status = COROUTINE_CONTEXT_RUNNING_STATUS_FINISH;
  
  // 提醒finish事件
  pthread_mutex_lock(&(context->sch->finish_context_mutex));
  context->sch->finish_context_number++;
  pthread_mutex_unlock(&(context->sch->finish_context_mutex));

  pthread_cond_broadcast(&(context->sch->finish_context_cond));
  return 0;
}

int ink_thread_destroy(ink_coroutine_thread_t *scht) {
  ink_context_destroy(scht->context);
  INK_COROUTINE_FREE(scht->context);
  return 0;
}

// 开始一个线程
void* ink_thread_run(ink_coroutine_t *ink_coroutine) {
  // 生成线程结构体
  ucontext_t* tc = (ucontext_t*)INK_COROUTINE_ALLOC(sizeof(ucontext_t));
  getcontext(tc);

  ink_coroutine_thread_t* scht = (ink_coroutine_thread_t*)INK_COROUTINE_ALLOC(sizeof(ink_coroutine_thread_t));
  scht->context = (ink_coroutine_context_t *)INK_COROUTINE_ALLOC(sizeof(ink_coroutine_context_t));
  scht->context->base_stack = NULL;
  scht->context->sch = ink_coroutine;
  scht->context->running = NULL;
  scht->context->base_context = tc;
  scht->base_thread = pthread_self();
  scht->status = COROUTINE_CONTEXT_THREAD_STATUS_RUNNING;
  
  pthread_mutex_lock(&(ink_coroutine->thread_mutex));
  int scht_id = ink_coroutine->thread_number;
  ink_coroutine->thread_list[ink_coroutine->thread_number++] = scht;
  pthread_mutex_unlock(&(ink_coroutine->thread_mutex));

  while (scht->status == COROUTINE_CONTEXT_THREAD_STATUS_RUNNING) {
    // 获得下一个准备执行的上下文
    ink_coroutine_context_t *next_context;
    block_list_pop(ink_coroutine->ready_context_list, (void **)&next_context);
    if (next_context != NULL) {
      ink_swap_in_context(next_context, scht);
    }
  }
  return NULL;
}

// 开始所有的线程
void ink_coroutine_thread_init(ink_coroutine_t *sch, int thread_num) {
  sch->thread_number = 0;
  for (int loop_i = 0; loop_i < thread_num; ++loop_i) {
    pthread_t thread;
    pthread_create(&thread, NULL, (void *(*) (void *))&ink_thread_run, sch);
  }
}

// 初始化
extern int ink_coroutine_init(ink_coroutine_t *sch, int thread_num) {
  sch->context_list = (ink_coroutine_context_t**)INK_COROUTINE_ALLOC(sizeof(ink_coroutine_context_t*) * LIST_MAX_SIZE);
  sch->context_number = 0;

  sch->thread_list = (ink_coroutine_thread_t**)INK_COROUTINE_ALLOC(sizeof(ink_coroutine_thread_t*) * LIST_MAX_SIZE);

  sch->ready_context_list = (block_list_t *)INK_COROUTINE_ALLOC(sizeof(block_list_t));
  block_list_init(sch->ready_context_list, LIST_MAX_SIZE);
  sch->finish_context_number = 0;

  pthread_cond_init(&(sch->wait_cond), NULL);
  pthread_mutex_init(&(sch->thread_mutex), NULL);
  pthread_cond_init(&(sch->finish_context_cond), NULL);
  pthread_mutex_init(&(sch->finish_context_mutex), NULL);

  ink_coroutine_thread_init(sch, thread_num);
  return 0;
}

int ink_context_defer_list_run(ink_coroutine_running_t *running) {
  int defer_list_size = cvector_size(running->running_context->defer_list);
  for (int loop_i = 0; loop_i < defer_list_size; ++loop_i) {
    running->running_context->defer_list[loop_i]->run_func(running, running->running_context->defer_list[loop_i]->args);
  }
  return 0;
}

// 运行程序的包装
void ink_context_wapper(ink_coroutine_running_t *running, ink_coroutine_run_func_t run_func, void *arg) {
  run_func(running, arg);
  ink_context_finish(running->running_context);

  // 运行defer的内容
  ink_context_defer_list_run(running);

  // swap to thread
  ink_swap_out_context(running->running_context, running->running_thread);
}

extern int ink_coroutine_run(ink_coroutine_t *sch, ink_coroutine_run_func_t run_func, void *arg) {
  ucontext_t *my_context = (ucontext_t *)INK_COROUTINE_ALLOC(sizeof(ucontext_t));
  getcontext(my_context);
  my_context->uc_stack.ss_size = STACK_SIZE;
  void *con_stack =  INK_COROUTINE_ALLOC(STACK_SIZE);
  my_context->uc_stack.ss_sp = con_stack;
  my_context->uc_link = NULL;
  ink_coroutine_running_t *running = (ink_coroutine_running_t *)INK_COROUTINE_ALLOC(sizeof(ink_coroutine_running_t));

  ink_coroutine_context_t *sct = (ink_coroutine_context_t *)INK_COROUTINE_ALLOC(sizeof(ink_coroutine_context_t));
  sct->base_stack = con_stack;
  sct->base_context = my_context;
  sct->running = running;
  sct->sch = sch;
  sct->running_status = COROUTINE_CONTEXT_RUNNING_STATUS_READY;
  sct->defer_list = NULL;
  running->running_context = sct;
  running->running_thread = NULL;
  running->sch = sch;
  makecontext(my_context, (void (*) (void))ink_context_wapper, 3, running, run_func, arg);

  pthread_mutex_lock(&(sch->thread_mutex));
  sch->context_list[sch->context_number] = sct;
  sch->context_number++;
  pthread_mutex_unlock(&(sch->thread_mutex));

  // 添加到ready_list
  block_list_push(sch->ready_context_list, sct);
  return 0;
}

extern int ink_coroutine_context_defer(ink_coroutine_running_t *running, ink_coroutine_run_func_t run_func, void *arg) {
  ink_coroutine_context_defer_callable_t *defer_callable = (ink_coroutine_context_defer_callable_t *)malloc(sizeof(ink_coroutine_context_defer_callable_t));
  defer_callable->run_func = run_func;
  defer_callable->args = arg;
  cvector_push_back(running->running_context->defer_list, defer_callable);
  return 0;
}

// 创建管道
extern int ink_coroutine_channel_init(ink_coroutine_channel_t *chan, int max_capacity) {
  chan->capacity = max_capacity;
  chan->data = list_new();
  chan->size = 0;
  chan->read_wait = list_new();
  chan->write_wait = list_new();
  chan->status = COROUTINE_CHANNEL_STATUS_ACTIVE;

  pthread_mutex_init(&(chan->mutex), NULL);
  return 0;
}

extern void ink_coroutine_channel_notify(ink_coroutine_t *scht, list_t *notify_list) {
  pthread_mutex_lock(&(scht->thread_mutex));
  list_node_t *ready_node;
  while ((ready_node = list_lpop(notify_list)) != NULL) {
    ink_coroutine_context_t *ready_context = (ink_coroutine_context_t *)(ready_node->val);
    ready_context->running_status = COROUTINE_CONTEXT_RUNNING_STATUS_READY;
    block_list_push(scht->ready_context_list, ready_context);
    LIST_FREE(ready_node);
  }
  pthread_mutex_unlock(&(scht->thread_mutex));
}

extern void *ink_coroutine_channel_pop(ink_coroutine_running_t *running, ink_coroutine_channel_t *chan) {
  pthread_mutex_lock(&(chan->mutex));
  while (chan->size <= 0 && chan->status == COROUTINE_CHANNEL_STATUS_ACTIVE) {
    // 挂起
    pthread_mutex_unlock(&(chan->mutex));
    list_node_t* wait_node = list_node_new(running->running_context);
    list_rpush(chan->read_wait, wait_node);

    ink_swap_out_context(running->running_context, running->running_thread);
    pthread_mutex_lock(&(chan->mutex));
  }

  // 管道关闭，直接返回NULL
  if (chan->status == COROUTINE_CHANNEL_STATUS_FINISH && chan->size == 0) {
    return NULL;
  }

  // 获取内容并清理现场
  list_node_t* result_node = list_lpop(chan->data);
  void *result = result_node->val;
  LIST_FREE(result_node);

  chan->size -= 1;
  ink_coroutine_channel_notify(running->sch, chan->write_wait);
  pthread_mutex_unlock(&(chan->mutex));
  return result;
}

extern int ink_coroutine_channel_push(ink_coroutine_running_t *running, ink_coroutine_channel_t* chan, void *data) {
  pthread_mutex_lock(&chan->mutex);

  // 判断是否阻塞（管道达到上限），如果阻塞，则挂起程序
  while (chan->size >= chan->capacity && chan->status == COROUTINE_CHANNEL_STATUS_ACTIVE) {
    // 挂起
    pthread_mutex_unlock(&(chan->mutex));
    list_node_t* wait_node = list_node_new(running->running_context);
    list_rpush(chan->write_wait, wait_node);

    ink_swap_out_context(running->running_context, running->running_thread);
    pthread_mutex_lock(&(chan->mutex));
  }

  // 管道关闭，无法写入
  if (chan->status == COROUTINE_CHANNEL_STATUS_FINISH) {
    return 1;
  }
  list_node_t *node_data = list_node_new(data);
  list_rpush(chan->data, node_data);

  chan->size += 1;
  ink_coroutine_channel_notify(running->sch, chan->read_wait);
  pthread_mutex_unlock(&(chan->mutex));
  return 0;
}

extern int ink_coroutine_join(ink_coroutine_t *sch) {
  pthread_mutex_lock(&(sch->finish_context_mutex));
  while (sch->finish_context_number != sch->context_number) {
    pthread_cond_wait(&(sch->finish_context_cond), &(sch->finish_context_mutex));
  }
  pthread_mutex_unlock(&(sch->finish_context_mutex));
  return 0;
}

extern int ink_coroutine_channel_close(ink_coroutine_running_t* running, ink_coroutine_channel_t* chan) {
  // 将管道标记为完成
  chan->status = COROUTINE_CHANNEL_STATUS_FINISH;

  // 将所有等待该管道的协程都移到等待区
  ink_coroutine_channel_notify(running->sch, chan->read_wait);
  ink_coroutine_channel_notify(running->sch, chan->write_wait);
  return 0;
}

extern int ink_coroutine_destroy(ink_coroutine_t *sch) {
  // 关闭线程
  for (int loop_i = 0; loop_i < sch->thread_number; ++loop_i) {
    sch->thread_list[loop_i]->status = COROUTINE_CONTEXT_THREAD_STATUS_FINISH;
  }

  for (int loop_i = 0; loop_i < sch->thread_number; ++loop_i) {
    block_list_push(sch->ready_context_list, NULL);
  }

  for (int loop_i = 0; loop_i < sch->thread_number; ++loop_i) {
    pthread_join(sch->thread_list[loop_i]->base_thread, NULL);
  }

  for (int loop_i = 0; loop_i < sch->thread_number; ++loop_i) {
    ink_thread_destroy(sch->thread_list[loop_i]);
    INK_COROUTINE_FREE(sch->thread_list[loop_i]);
  }
  pthread_mutex_destroy(&(sch->thread_mutex));
  INK_COROUTINE_FREE(sch->thread_list);

  for (int loop_i = 0; loop_i < sch->context_number; ++loop_i) {
    ink_context_destroy(sch->context_list[loop_i]);
    INK_COROUTINE_FREE(sch->context_list[loop_i]);
  }
  pthread_mutex_destroy(&(sch->finish_context_mutex));
  pthread_cond_destroy(&(sch->finish_context_cond));
  pthread_cond_destroy(&(sch->wait_cond));
  INK_COROUTINE_FREE(sch->context_list);  

  // 清理各种list
  block_list_destroy(sch->ready_context_list, NULL);
  INK_COROUTINE_FREE(sch->ready_context_list);

  return 0;
}

extern int ink_coroutine_channel_destroy(ink_coroutine_channel_t *chan) {
  pthread_mutex_destroy(&(chan->mutex));
  list_destroy(chan->data);
  list_destroy(chan->read_wait);
  list_destroy(chan->write_wait);
  return 0;
}
