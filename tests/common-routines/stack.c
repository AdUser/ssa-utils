#include "../../src/common.h"
#include <assert.h>
#include <signal.h>

static void handler(int sig, siginfo_t *siginfo, void *context)
{ exit(EXIT_FAILURE); }

int main()
{
  struct sigaction act;
  STACK_ELEM stack[STACK_MAX];
  STACK_ELEM *top = stack;
  void *tmp;

  memset (&act, '\0', sizeof(act));
  act.sa_sigaction = &handler;
  sigaction(SIGABRT, &act, NULL);

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

  assert(true == false);

  return 0;
}
