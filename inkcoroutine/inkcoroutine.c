#include "inkcoroutine.h"

#include <stdio.h>
#include <stdlib.h>

#define LIST_MAX_SIZE 1024

int ink_swap_context(schedule_context_t* save_context, const schedule_context_t* to_context) {
  return swapcontext((save_context->base_context), (to_context->base_context));
}

int ink_thread_run(schedule_t *schedule) {
  // 生成线程结构体
  ucontext_t* tc = (ucontext_t*)malloc(sizeof(ucontext_t));
  getcontext(tc);
  schedule_thread_t* scht = (schedule_thread_t*)malloc(sizeof(schedule_thread_t));
  scht->context.base_context = tc;
  scht->base_thread = pthread_self();
  scht->continue_flag = 1;
  
  pthread_mutex_lock(&(schedule->thread_mutex));
  int scht_id = schedule->thread_number;
  schedule->thread_list[schedule->thread_number++] = scht;
  pthread_mutex_unlock(&(schedule->thread_mutex));

  while (scht->continue_flag) {
    // 获得下一个准备执行的上下文
    schedule_context_t *next_context;
    ink_list_pop(schedule->wait_context_list, (void **)&next_context);
    next_context->run_thread = scht;

    ink_swap_context(&(scht->context), next_context);
  }
  return 0;
}

// 初始化
extern schedule_t* schedule_init() {
  schedule_t* sch = (schedule_t *)malloc(sizeof(schedule_t));
  sch->context_list = (schedule_context_t**)malloc(sizeof(schedule_context_t*) * LIST_MAX_SIZE);
  sch->context_number = 0;

  sch->thread_list = (schedule_thread_t**)malloc(sizeof(schedule_thread_t*) * LIST_MAX_SIZE);
  sch->thread_number = 0;

  sch->pipeline_list = (schedule_pipeline_t**)malloc(sizeof(schedule_pipeline_t*) * LIST_MAX_SIZE);
  sch->pipeline_number = 0;

  sch->wait_context_list = (ink_list_t *)malloc(sizeof(ink_list_t));
  ink_list_init(sch->wait_context_list, LIST_MAX_SIZE);

  pthread_mutex_init(&(sch->thread_mutex), NULL);
  return sch;
}
