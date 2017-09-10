#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../src/dynamic/list.h"

static void release(void *object)
{
  free(*(char **) object);
}

static int compare(void *a, void *b)
{
  char *sa = *(char **) a, *sb = *(char **) b;

  return strcmp(sa, sb);
}

void core()
{
  list l;
  char *a[] = {"a", "list", "of", "string", "pointers"}, **s;
  size_t i;

  list_construct(&l);
  for (i = 0; i < sizeof a / sizeof *a; i ++)
    list_push_back(&l, &a[i], sizeof (char *));

  s = list_begin(&l);
  assert_string_equal(*s, "a");


  list_push_front(&l, (char*[]){"test"}, sizeof (char *));
  s = list_front(&l);
  assert_string_equal(*s, "test");
  list_erase(s, NULL);

  i = 0;
  list_foreach(&l, s)
    assert_string_equal(*s, a[i ++]);

  i = sizeof a / sizeof *a;
  list_foreach_reverse(&l, s)
    assert_string_equal(*s, a[-- i]);

  s = list_find(&l, compare, (char*[]){"pointers"});
  assert_string_equal(*s, "pointers");

  s = list_find(&l, compare, (char*[]){"foo"});
  assert_true(!s);

  list_destruct(&l, NULL);
}

void alloc()
{
  list l;
  char *s;

  list_construct(&l);

  s = strdup("1");
  list_push_back(&l, &s, sizeof s);
  s = strdup("2");
  list_push_back(&l, &s, sizeof s);
  s = strdup("3");
  list_push_back(&l, &s, sizeof s);

  list_destruct(&l, release);
}

void unit()
{
  list l;
  int *p;

  list_construct(&l);

  list_insert(list_front(&l), (int[]){1}, sizeof (int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_clear(&l, NULL);

  list_insert(list_previous(list_front(&l)), (int[]){1}, sizeof (int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_erase(list_back(&l), NULL);

  list_push_front(&l, (int[]){1}, sizeof (int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_erase(list_front(&l), NULL);

  list_push_back(&l, (int[]){1}, sizeof (int));
  assert_int_equal(*(int *) list_front(&l), 1);
  list_clear(&l, NULL);

  list_push_back(&l, (int[]){1}, sizeof (int));
  list_push_back(&l, (int[]){2}, sizeof (int));
  list_push_back(&l, (int[]){3}, sizeof (int));
  p = list_next(list_front(&l));
  assert_int_equal(*p, 2);
  list_erase(p, NULL);
  p = list_next(list_front(&l));
  assert_int_equal(*p, 3);

  list_destruct(&l, NULL);
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
    cmocka_unit_test(alloc),
    cmocka_unit_test(unit)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

