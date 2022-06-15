#ifndef __INK_WAIT_GROUP
#define __INK_WAIT_GROUP

#include <pthread.h>

typedef struct ink_wait_group_t {
  int wait_number;
  int finish_number;
  
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} ink_wait_group_t;

extern int ink_wait_group_init(ink_wait_group_t *wg);
extern int ink_wait_group_add(ink_wait_group_t *wg, int num);
extern int ink_wait_group_done(ink_wait_group_t *wg);
extern int ink_wait_group_wait(ink_wait_group_t *wg);
extern int ink_wait_group_destroy(ink_wait_group_t *wg);

#endif