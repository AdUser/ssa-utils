#include "../tests.h"

int main()
{
  struct slist *list = NULL;

  SIGCATCH_INIT

  assert(slist_find(list, "test") == false);

  /* wrong flag combinations */
  assert(slist_add(&list, "test", 0) == false);
  assert(slist_add(&list, "test", SLIST_FIRST | SLIST_LAST) == false);
  assert(list == NULL);

  /* simple add */
  assert(slist_add(&list, "test", SLIST_LAST) == true);
  assert(strcmp(list->value, "test") == 0);
  assert(list->next == NULL);

  /* testing slist_find for one-item list */
  assert(slist_find(list, "test")  == list);
  assert(slist_find(list, "wrong") == NULL);

  /* uniq add */
  assert(slist_add(&list, "test", SLIST_FIRST | SLIST_MOD_UNIQ) == true);
  assert(slist_add(&list, "test", SLIST_LAST  | SLIST_MOD_UNIQ) == true);
  assert(list->next == NULL);

  /* uniq add - force */
  assert(slist_add(&list, "test", SLIST_FIRST | SLIST_MOD_UNIQ | SLIST_MOD_CRTNLY) == false);
  assert(slist_add(&list, "test", SLIST_LAST  | SLIST_MOD_UNIQ | SLIST_MOD_CRTNLY) == false);
  assert(list->next == NULL);

  /* non-uniq add */
  assert(slist_add(&list, "test", SLIST_LAST) == true);
  assert(list->next != NULL && list->next->next == NULL);
  assert(strcmp(list->next->value, "test") == 0);
  assert(slist_find(list, "test") == list);

  assert(slist_del(&list, "test", SLIST_MOD_ALL | SLIST_MOD_MATCH) == true);
  assert(list == NULL);

  /* test order of items */
  assert(slist_add(&list, "test", 0) == false); /* no flags */
  assert(slist_add(&list, "test",   SLIST_FIRST) == true);
  assert(slist_add(&list, "after",  SLIST_LAST)  == true);
  assert(slist_add(&list, "before", SLIST_FIRST) == true);
  assert(slist_find(list, "before") == list);
  assert(slist_find(list, "test")   == list->next);
  assert(slist_find(list, "after")  == list->next->next);

  /* delete item in middle of list */
  assert(slist_del(&list, "test", SLIST_MOD_ALL | SLIST_MOD_MATCH) == true);
  assert(slist_find(list, "before") == list);
  assert(slist_find(list, "after")  == list->next);

  /* delete all remaining items in list */
  assert(slist_del(&list, "", SLIST_MOD_ALL) == true);
  assert(list == NULL);

  assert(slist_add(&list, "three", SLIST_FIRST) == true);
  assert(slist_add(&list, "two",   SLIST_FIRST) == true);
  assert(slist_add(&list, "one",   SLIST_FIRST) == true);

  assert(slist_del(&list, "four", SLIST_MOD_MATCH) == false);
  assert(slist_del(&list, "four", SLIST_MOD_MATCH | SLIST_FIRST) == true);
  assert(slist_del(&list, "four", SLIST_MOD_MATCH | SLIST_LAST) == true);

  assert(slist_del(&list, "one",   SLIST_MOD_MATCH | SLIST_LAST) == true);
  assert(slist_del(&list, "three", SLIST_MOD_MATCH | SLIST_LAST) == true);
  assert(slist_find(list, "two") == list);
  assert(list->next == NULL);

  assert(slist_add(&list, "three", SLIST_LAST) == true);
  assert(slist_add(&list, "one",   SLIST_FIRST) == true);

  assert(slist_del(&list, "one",   SLIST_MOD_MATCH | SLIST_FIRST) == true);
  assert(slist_del(&list, "three", SLIST_MOD_MATCH | SLIST_FIRST) == true);
  assert(slist_find(list, "two") == list);
  assert(list->next == NULL);

  assert(slist_add(&list, "three", SLIST_LAST) == true);
  assert(slist_add(&list, "one",   SLIST_FIRST) == true);

  assert(slist_del(&list, "", SLIST_FIRST) == true);
  assert(slist_del(&list, "", SLIST_LAST)  == true);
  assert(slist_find(list, "two") == list);
  assert(list->next == NULL);

  return 0;
}
