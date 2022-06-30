#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include "inkcoroutine.h"
#include "wait_group.h"
#include <unistd.h>

ink_wait_group_t wg;

void defer_it(ink_coroutine_running_t *r) {
  ink_wait_group_done(&wg);
}

int run_context1(ink_coroutine_running_t *a, ink_coroutine_channel_t *p) {
  ink_coroutine_context_defer(a, defer_it, NULL);
  int *pi;
  while ((pi = ink_coroutine_channel_pop(a, p)) != NULL) {
    printf("%d\n", *pi);
    free(pi);
  }
  printf("Hello\n");
  return 0;
}

int run_context2(ink_coroutine_running_t *a, ink_coroutine_channel_t *p) {
  ink_coroutine_context_defer(a, defer_it, NULL);
  int *data = (void *)malloc(sizeof(int));
  *data = 123;
  ink_coroutine_channel_push(a, p, data);
  ink_coroutine_channel_close(a, p);
  return 0;
}

int main() {
  ink_wait_group_init(&wg);
  ink_coroutine_t sch;
  ink_coroutine_init(&sch, 1);
  ink_coroutine_channel_t chan1;
  ink_coroutine_channel_init(&chan1, 1);
  ink_wait_group_add(&wg, 1);
  ink_coroutine_run(&sch, run_context1, &chan1);
  ink_wait_group_add(&wg, 1);
  ink_coroutine_run(&sch, run_context2, &chan1);
  ink_wait_group_wait(&wg);
  ink_wait_group_destroy(&wg);
  ink_coroutine_destroy(&sch);
  ink_coroutine_channel_destroy(&chan1);
  return 0;
}
