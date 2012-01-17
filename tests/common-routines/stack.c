#include "../tests.h"

int main()
{
  STACK_ELEM stack[STACK_MAX];
  STACK_ELEM *top = stack;
  void *tmp;

  SIGCATCH_INIT

  assert(sizeof(STACK_ELEM) == STACK_ELEM_SIZE);

  stack_init(stack);
  CALLOC(tmp, STACK_MAX, STACK_ELEM_SIZE);
  assert(memcmp(stack, tmp, 0) == 0);
  free(tmp);

  assert(*top == '\0');

  stack_push(stack, &top, 'A');
  assert(*top == 'A');

  stack_push(stack, &top, 'B');
  assert(*top == 'B');

  stack_pop(stack, &top);
  assert(*top == 'A');

  stack_pop(stack, &top);
  assert(*top == '\0');

  stack_pop(stack, &top);
  assert(*top == '\0');

  return 0;
}
