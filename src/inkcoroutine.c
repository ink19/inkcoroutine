#include "inkcoroutine.h"

#include <stdio.h>
#include <stdlib.h>

#define LIST_MAX_SIZE 1024
#define STACK_SIZE (10 * 1024 * 1024)


int ink_swap_context(schedule_context_t* save_context, const schedule_context_t* to_context) {
  return swapcontext((save_context->base_context), (to_context->base_context));
}

int ink_swap_out_context(schedule_context_t *save_context, schedule_thread_t* thread_obj) {
  save_context->running->running_thread = NULL;
  save_context->running_status = SCHEDULE_CONTEXT_RUNNING_STATUS_WAIT;
  ink_swap_context(save_context, thread_obj->context);
  return 0;
}

int ink_swap_in_context(schedule_context_t *save_context, schedule_thread_t* thread_obj) {
  save_context->running->running_thread = thread_obj;
  save_context->running_status = SCHEDULE_CONTEXT_RUNNING_STATUS_RUNNING;
  ink_swap_context(thread_obj->context, save_context);
  return 0;
}

int ink_context_finish(schedule_context_t *context) {
  context->running_status = SCHEDULE_CONTEXT_RUNNING_STATUS_FINISH;
  
  // 提醒finish事件
  pthread_mutex_lock(&(context->sch->finish_context_mutex));
  context->sch->finish_context_number++;
  pthread_mutex_unlock(&(context->sch->finish_context_mutex));

  pthread_cond_broadcast(&(context->sch->finish_context_cond));
  return 0;
}

void* ink_thread_run(schedule_t *schedule) {
  // 生成线程结构体
  ucontext_t* tc = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(tc);

  schedule_thread_t* scht = (schedule_thread_t*)malloc(sizeof(schedule_thread_t));
  scht->context = (schedule_context_t *)malloc(sizeof(schedule_context_t));
  scht->context->base_context = tc;
  scht->base_thread = pthread_self();
  scht->continue_flag = 1;
  
  pthread_mutex_lock(&(schedule->thread_mutex));
  int scht_id = schedule->thread_number;
  schedule->thread_list[schedule->thread_number++] = scht;
  pthread_mutex_unlock(&(schedule->thread_mutex));

  while (scht->continue_flag) {
    // 获得下一个准备执行的上下文
    schedule_context_t *next_context;
    ink_list_pop(schedule->ready_context_list, (void **)&next_context);
    
    ink_swap_in_context(next_context, scht);
  }
  return NULL;
}

void schedule_thread_init(schedule_t *sch) {
  for (int loop_i = 0; loop_i < sch->thread_number; ++loop_i) {
    pthread_t thread;
    pthread_create(&thread, NULL, ink_thread_run, sch);
  }
}

// 初始化
extern schedule_t* schedule_init(int thread_num) {
  schedule_t* sch = (schedule_t *)malloc(sizeof(schedule_t));
  sch->context_list = (schedule_context_t**)malloc(sizeof(schedule_context_t*) * LIST_MAX_SIZE);
  sch->context_number = 0;

  sch->thread_list = (schedule_thread_t**)malloc(sizeof(schedule_thread_t*) * LIST_MAX_SIZE);
  sch->thread_number = thread_num;

  sch->pipeline_list = (schedule_channel_t**)malloc(sizeof(schedule_channel_t*) * LIST_MAX_SIZE);
  sch->pipeline_number = 0;

  sch->ready_context_list = (ink_list_t *)malloc(sizeof(ink_list_t));
  ink_list_init(sch->ready_context_list, LIST_MAX_SIZE);

  pthread_cond_init(&(sch->wait_cond), NULL);
  pthread_mutex_init(&(sch->thread_mutex), NULL);
  pthread_cond_init(&(sch->finish_context_cond), NULL);
  pthread_mutex_init(&(sch->finish_context_mutex), NULL);

  schedule_thread_init(sch);
  return sch;
}

void ink_context_wapper(schedule_running_t *running, schedule_run_func_t run_func, void *arg) {
  printf("Begin Context\n");
  run_func(running, arg);
  ink_context_finish(running->running_context);
  // swap to thread
  ink_swap_out_context(running->running_context, running->running_thread);
}

extern void schedule_run(schedule_t *sch, schedule_run_func_t run_func, void *arg) {
  ucontext_t *my_context = (ucontext_t *)malloc(sizeof(ucontext_t));
  getcontext(my_context);
  my_context->uc_stack.ss_size = STACK_SIZE;
  my_context->uc_stack.ss_sp = malloc(STACK_SIZE);
  my_context->uc_link = NULL;
  schedule_running_t *running = (schedule_running_t *)malloc(sizeof(schedule_running_t));

  schedule_context_t *sct = (schedule_context_t *)malloc(sizeof(schedule_context_t));
  sct->base_context = my_context;
  sct->running = running;
  sct->sch = sch;
  sct->running_status = SCHEDULE_CONTEXT_RUNNING_STATUS_READY;
  running->running_context = sct;
  running->running_thread = NULL;
  running->sch = sch;
  makecontext(my_context, ink_context_wapper, 3, running, run_func,arg);

  pthread_mutex_lock(&(sch->thread_mutex));
  sch->context_list[sch->context_number] = sct;
  sch->context_number++;
  pthread_mutex_unlock(&(sch->thread_mutex));

  // 添加到ready_list
  ink_list_push(sch->ready_context_list, sct);
}

// 创建管道
extern schedule_channel_t *schedule_channel_init(int max_capacity) {
  schedule_channel_t *result = (schedule_channel_t *)malloc(sizeof(schedule_channel_t));
  result->capacity = max_capacity;
  result->data = list_new();
  result->size = 0;
  result->read_wait = list_new();
  result->write_wait = list_new();

  pthread_mutex_init(&(result->mutex), NULL);
  return result;
}

extern void schedule_channel_notify(schedule_t *scht, list_t *notify_list) {
  pthread_mutex_lock(&(scht->thread_mutex));
  list_node_t *ready_node;
  while ((ready_node = list_lpop(notify_list)) != NULL) {
    schedule_context_t *ready_context = (schedule_context_t *)(ready_node->val);
    ready_context->running_status = SCHEDULE_CONTEXT_RUNNING_STATUS_READY;
    ink_list_push(scht->ready_context_list, ready_context);
    LIST_FREE(ready_node);
  }
  pthread_mutex_unlock(&(scht->thread_mutex));
}

extern void *schedule_channel_pop(schedule_running_t *running, schedule_channel_t *chan) {
  pthread_mutex_lock(&(chan->mutex));
  while (chan->size <= 0) {
    // 挂起
    pthread_mutex_unlock(&(chan->mutex));
    list_node_t* wait_node = list_node_new(running->running_context);
    list_rpush(chan->read_wait, wait_node);

    ink_swap_out_context(running->running_context, running->running_thread);
    pthread_mutex_lock(&(chan->mutex));
  }
  
  // 获取内容并清理现场
  list_node_t* result_node = list_lpop(chan->data);
  void *result = result_node->val;
  LIST_FREE(result_node);

  chan->size -= 1;
  schedule_channel_notify(running->sch, chan->write_wait);
  pthread_mutex_unlock(&(chan->mutex));
  return result;
}

extern void schedule_channel_push(schedule_running_t*running, schedule_channel_t* chan, void *data) {
  pthread_mutex_lock(&chan->mutex);

  while (chan->size >= chan->capacity) {
    // 挂起
    pthread_mutex_unlock(&(chan->mutex));
    list_node_t* wait_node = list_node_new(running->running_context);
    list_rpush(chan->write_wait, wait_node);

    ink_swap_out_context(running->running_context, running->running_thread);
    pthread_mutex_lock(&(chan->mutex));
  }
  list_node_t *node_data = list_node_new(data);
  list_rpush(chan->data, node_data);

  chan->size += 1;
  schedule_channel_notify(running->sch, chan->read_wait);
  pthread_mutex_unlock(&(chan->mutex));
  return;
}

extern void schedule_join(schedule_t *sch) {
  pthread_mutex_lock(&(sch->finish_context_mutex));
  while (sch->finish_context_number != sch->context_number) {
    pthread_cond_wait(&(sch->finish_context_cond), &(sch->finish_context_mutex));
  }
  pthread_mutex_unlock(&(sch->finish_context_mutex));
}