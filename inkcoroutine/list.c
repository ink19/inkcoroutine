#include "list.h"

#include <stdlib.h>

extern int ink_list_init(ink_list_t* list, int capacity) {
  pthread_mutex_init(&(list->_rw_mutex), NULL);
  pthread_cond_init(&(list->_rw_cond), NULL);
  list->_list.next = NULL;
  list->_size = 0;
  list->_capacity = capacity;
  list->_final_node = &list->_list;
  return 0;
}

extern int ink_list_push(ink_list_t *list, void *data) {
  pthread_mutex_lock(&(list->_rw_mutex));
  while (list->_size >= list->_capacity) {
    pthread_cond_wait(&(list->_rw_cond), &(list->_rw_mutex));
  }

  ink_list_item_t* tnode = (ink_list_item_t *)malloc(sizeof(ink_list_item_t));
  tnode->next = NULL;
  tnode->data = data;
  list->_final_node->next = tnode;
  list->_final_node = tnode;
  list->_size += 1;
  pthread_mutex_unlock(&(list->_rw_mutex));
  pthread_cond_broadcast(&(list->_rw_cond));

  return 0;
}

extern int ink_list_pop(ink_list_t* list, void **data) {
  pthread_mutex_lock(&(list->_rw_mutex));
  while(list->_size > 0) {
    pthread_cond_wait(&(list->_rw_cond), &(list->_rw_mutex));
  }

  ink_list_item_t* tnode = list->_list.next->data;
  list->_list.next = tnode->next;
  *data = tnode->data;
  list->_size--;
  free(tnode);

  pthread_mutex_unlock(&(list->_rw_mutex));
  pthread_cond_broadcast(&(list->_rw_cond));
  return 0;
}

extern int ink_list_size(const ink_list_t* list) {
  return list->_size;
}
