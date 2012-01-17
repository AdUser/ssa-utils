#include "../src/common.h"
#include <assert.h>
#include <signal.h>

static void handler(int sig, siginfo_t *siginfo, void *context)
{ exit(EXIT_FAILURE); }

#define SIGCATCH_INIT \
  struct sigaction act; \
  memset (&act, '\0', sizeof(act)); \
  act.sa_sigaction = &handler; \
  sigaction(SIGABRT, &act, NULL);
