#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include "inkcoroutine.h"
#include <unistd.h>

int run_context1(schedule_running_t *a, schedule_channel_t *p) {
  int *pi;
  while ((pi = schedule_channel_pop(a, p)) != NULL) {
    printf("%d\n", *pi);
    free(pi);
  }
  printf("Hello\n");
  return 0;
}

int run_context2(schedule_running_t *a, schedule_channel_t *p) {
  int *data = (void *)malloc(sizeof(int));
  *data = 123;
  schedule_channel_push(a, p, data);
  schedule_channel_close(a, p);
  return 0;
}

int main() {
  schedule_t *sch = schedule_init(1);
  schedule_channel_t *chan1 = schedule_channel_init(1);

  schedule_run(sch, run_context1, chan1);
  schedule_run(sch, run_context2, chan1);
  schedule_join(sch);
  return 0;
}
