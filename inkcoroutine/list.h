#ifndef _INKCOROUTINE_LIST_H
#define _INKCOROUTINE_LIST_H

#include <pthread.h>

typedef struct ink_list_item_t {
  void *data;
  struct ink_list_item_t* next;
} ink_list_item_t;

typedef struct ink_list_t {
  int _size;
  int _capacity;
  // rm lock
  pthread_mutex_t _rw_mutex;
  pthread_cond_t _rw_cond;

  ink_list_item_t _list;
  ink_list_item_t *_final_node;
} ink_list_t;

extern int ink_list_init(ink_list_t* list, int capacity);
extern int ink_list_push(ink_list_t* list, void *data);
extern int ink_list_pop(ink_list_t* list, void **data);
extern int ink_list_size(const ink_list_t* list);

#endif