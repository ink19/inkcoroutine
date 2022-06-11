#include "list.h"
#include <check.h>
#include <stdlib.h>

Suite *list_test() {
  Suite *s;
  TCase *core;

  s = suite_create("List");

  core = tcase_create("Core");
  suite_add_tcase(s, core);
  
  return s;
}

int main() {
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = list_test();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
  return 0;
}