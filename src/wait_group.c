#include "wait_group.h"

#include <stdlib.h>
#include <stdio.h>

extern int ink_wait_group_init(ink_wait_group_t *wg) {
  wg->wait_number = 0;
  wg->finish_number = 0;

  pthread_mutex_init(&(wg->mutex), NULL);
  pthread_cond_init(&(wg->cond), NULL);
  return 0;
}

extern int ink_wait_group_add(ink_wait_group_t *wg, int num) {
  pthread_mutex_lock(&(wg->mutex));
  wg->wait_number += num;
  pthread_mutex_unlock(&(wg->mutex));
  return wg->wait_number;
}

extern int ink_wait_group_done(ink_wait_group_t *wg) {
  pthread_mutex_lock(&(wg->mutex));
  wg->finish_number++;
  pthread_mutex_unlock(&(wg->mutex));
  pthread_cond_broadcast(&(wg->cond));
  return wg->finish_number;
}

extern int ink_wait_group_wait(ink_wait_group_t *wg) {
  pthread_mutex_lock(&(wg->mutex));
  while (wg->finish_number != wg->wait_number) {
    pthread_cond_wait(&(wg->cond), &(wg->mutex));
  }
  pthread_mutex_unlock(&(wg->mutex));
  return 0;
}

extern int ink_wait_group_destroy(ink_wait_group_t *wg) {
  pthread_mutex_destroy(&(wg->mutex));
  pthread_cond_destroy(&(wg->cond));
  return 0;
}
