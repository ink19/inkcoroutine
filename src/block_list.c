#include "block_list.h"

#include <stdlib.h>

extern int block_list_init(block_list_t* list, int capacity) {
  pthread_mutex_init(&(list->_rw_mutex), NULL);
  pthread_cond_init(&(list->_rw_cond), NULL);
  list->_list.next = NULL;
  list->_size = 0;
  list->_capacity = capacity;
  list->_final_node = &(list->_list);
  return 0;
}

extern int block_list_push(block_list_t *list, void *data) {
  pthread_mutex_lock(&(list->_rw_mutex));
  while (list->_size >= list->_capacity) {
    pthread_cond_wait(&(list->_rw_cond), &(list->_rw_mutex));
  }

  block_list_item_t* tnode = (block_list_item_t *)malloc(sizeof(block_list_item_t));
  tnode->next = NULL;
  tnode->data = data;
  list->_final_node->next = tnode;
  list->_final_node = tnode;
  list->_size += 1;

  pthread_mutex_unlock(&(list->_rw_mutex));
  pthread_cond_broadcast(&(list->_rw_cond));
  return 0;
}

extern int block_list_pop(block_list_t* list, void **data) {
  pthread_mutex_lock(&(list->_rw_mutex));
  while(list->_size <= 0) {
    pthread_cond_wait(&(list->_rw_cond), &(list->_rw_mutex));
  }

  block_list_item_t* tnode = list->_list.next;
  list->_list.next = tnode->next;
  *data = tnode->data;

  list->_size -= 1;
  if (list->_size == 0) {
    list->_final_node = &(list->_list);
  }
  free(tnode);

  pthread_mutex_unlock(&(list->_rw_mutex));
  pthread_cond_broadcast(&(list->_rw_cond));
  return 0;
}

extern int block_list_size(const block_list_t* list) {
  return list->_size;
}

extern int block_list_destroy(block_list_t* list, list_destroy_item destroy_func) {
  // 清除内容，防泄露
  while (block_list_size(list) != 0) {
    void *data;
    block_list_pop(list, data);
    if (destroy_func != NULL) {
      destroy_func(data);
    }
  }

  pthread_mutex_destroy(&(list->_rw_mutex));
  pthread_cond_destroy(&(list->_rw_cond));
  list->_size = 0;
  list->_capacity = 0;
  list->_final_node = &list->_list;
  list->_list.next = NULL;
  return 0;
}

extern int block_list_capacity(const block_list_t* list) {
  return list->_capacity;
}
