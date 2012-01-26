#include "../tests.h"

int main()
{
  struct slist *list = NULL;

  SIGCATCH_INIT

  /* wrong flag combinations */
  assert(slist_add(&list, "test", 0) == false);
  assert(slist_add(&list, "test", SLIST_ADD_UNIQ) == false);

  /* simple add */
  assert(slist_add(&list, "test", SLIST_ADD_LAST) == true);
  assert(strcmp(list->value, "test") == 0);
  assert(slist_find(list, "test") == list);

  /* uniq add */
  assert(slist_add(&list, "test", SLIST_ADD_UNIQ | SLIST_ADD_FIRST) == true);
  assert(slist_add(&list, "test", SLIST_ADD_UNIQ | SLIST_ADD_LAST) == true);
  assert(list->next == NULL);

  /* non-uniq add */
  slist_add(&list, "test", SLIST_ADD_LAST);
  assert(list->next != NULL);
  assert(strcmp(list->next->value, "test") == 0);
  assert(list->next->next == NULL);
  assert(slist_find(list, "test") == list);
  FREE(list->next->value);
  FREE(list->next);

  assert(slist_del(&list, "test", SLIST_DEL_ALL_MATCH) == true);
  assert(list == NULL);

  /* test order of items */
  slist_add(&list, "test", 0); /* no flags */
  assert(slist_add(&list, "after",  SLIST_ADD_LAST)  == true);
  assert(slist_add(&list, "before", SLIST_ADD_FIRST) == true);
  assert(slist_find(list, "before") == list);
  assert(slist_find(list, "test")   == list->next);
  assert(slist_find(list, "after")  == list->next->next);

  assert(slist_del(&list, "test", 0) == true);
  assert(slist_find(list, "before") == list);
  assert(slist_find(list, "after")  == list->next);

  assert(slist_del(&list, "", SLIST_DEL_FIRST) == true);
  assert(slist_find(list, "after") == list);
  assert(slist_add(&list, "test", SLIST_ADD_FIRST) == true);
  assert(slist_del(&list, "", SLIST_DEL_LAST) == true);
  assert(slist_find(list, "after") == NULL);

  return 0;
}
