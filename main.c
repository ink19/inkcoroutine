#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include "inkcoroutine.h"
#include <unistd.h>

int run_context1(schedule_running_t *a, schedule_channel_t *p) {
  void *data = schedule_channel_pop(a, p);
  printf("%d\n", data);
  return 0;
}

int run_context2(schedule_running_t *a, schedule_channel_t *p) {
  schedule_channel_push(a, p, 123);
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