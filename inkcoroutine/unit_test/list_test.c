#include "list.h"
#include <check.h>
#include <stdlib.h>

ink_list_t test_list;
int test_list_capacity = 5;

void test_list_init() {
  ink_list_init(&test_list, test_list_capacity);
}

void test_list_destroy() {
  ink_list_destroy(&test_list, NULL);
}

START_TEST(list_verify_capacity_and_size) {
  ck_assert_int_eq(ink_list_capacity(&test_list), 5);
  ck_assert_int_eq(ink_list_size(&test_list), 0);
}
END_TEST

START_TEST(list_push_test) {
  ink_list_push(&test_list, (void *)710);
  ck_assert_int_eq(ink_list_size(&test_list), 1);
}
END_TEST

START_TEST(list_pop_test) {
  void *data;
  ink_list_push(&test_list, (void *)710);
  ck_assert_int_eq(ink_list_size(&test_list), 1);
  ink_list_pop(&test_list, &data);
  ck_assert_msg(data ==(void *) 710, "");
}
END_TEST

Suite *list_test() {
  Suite *s;
  TCase *core;

  s = suite_create("List");

  core = tcase_create("Core");
  tcase_add_unchecked_fixture(core, test_list_init, test_list_destroy);
  tcase_add_test(core, list_verify_capacity_and_size);
  tcase_add_test(core, list_push_test);
  tcase_add_test(core, list_pop_test);
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