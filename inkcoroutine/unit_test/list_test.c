#include "list.h"
#include <check.h>

START_TEST(LIST_INIT) {
  ink_list_t list;
  ink_list_init(&list, 10);
  ck_assert_int_eq(list._capacity, 10);
}
END_TEST

int main() {
  return 0;
}