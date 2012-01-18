#include "../tests.h"

int main()
{
  struct slist *list = NULL;
  struct slist *p = NULL;

  SIGCATCH_INIT

  /* wrong flag combinations */
  assert(slist_add(&list, "test", 0) == false);
  assert(slist_add(&list, "test", SLIST_ADD_UNIQ) == false);

  /* simple add */
  assert(slist_add(&list, "test", SLIST_ADD_LAST) == true);
  assert(strcmp(list->value, "test") == 0);

  /* uniq add */
  assert(slist_add(&list, "test", SLIST_ADD_UNIQ | SLIST_ADD_FIRST) == true);
  assert(slist_add(&list, "test", SLIST_ADD_UNIQ | SLIST_ADD_LAST) == true);
  assert(list->next == NULL);

  /* non-uniq add */
  slist_add(&list, "test", SLIST_ADD_LAST);
  assert(list->next != NULL);
  assert(strcmp(list->next->value, "test") == 0);
  assert(list->next->next == NULL);
  FREE(list->next->value);
  FREE(list->next);

  /* test order of items */
  assert(slist_add(&list, "after",  SLIST_ADD_LAST)  == true);
  assert(slist_add(&list, "before", SLIST_ADD_FIRST) == true);

  p = list;
  assert(strcmp(p->value, "before") == 0);
  assert((p = p->next) != NULL);
  assert(strcmp(p->value, "test")   == 0);
  assert((p = p->next) != NULL);
  assert(strcmp(p->value, "after")  == 0);
  assert((p = p->next) == NULL);

  return 0;
}
