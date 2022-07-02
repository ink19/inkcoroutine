#ifndef _INKCOROUTINE_LIST_H
#define _INKCOROUTINE_LIST_H

#include <pthread.h>

typedef struct block_list_item_t {
  void *data;
  struct block_list_item_t* next;
} block_list_item_t;

typedef struct block_list_t {
  int _size;
  int _capacity;
  // rm lock
  pthread_mutex_t _rw_mutex;
  pthread_cond_t _rw_cond;

  block_list_item_t _list;
  block_list_item_t *_final_node;
} block_list_t;

typedef int (*list_destroy_item)(void *);

// 初始化
extern int block_list_init(block_list_t* list, int capacity);

// 入栈
extern int block_list_push(block_list_t* list, void *data);

// 出栈
extern int block_list_pop(block_list_t* list, void **data);

// 销毁
extern int block_list_destroy(block_list_t* list, list_destroy_item destroy_func);
extern int block_list_size(const block_list_t* list);
extern int block_list_capacity(const block_list_t* list);

#endif
