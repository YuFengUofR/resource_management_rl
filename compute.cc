#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/stat.h"
#include "sys/types.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "signal.h"
#include "probe.h"

Probe* probe;
void sigusr2_handler(int sig);

int main(int argc, char **argv)
{
  unsigned long long num = atoi(argv[1]);
  double ret = 0.0;

  signal(SIGUSR2, sigusr2_handler);
  sigset_t ourmask;
  probe = new Probe();
  probe->init();
  probe->start();

  sigfillset(&ourmask);  // make a full-blocked mask;
  sigprocmask(SIG_UNBLOCK, &ourmask, NULL);   // switch mask

  for (unsigned long long i = 0; i < num; i++) {
    ret *= (double) i;
    if (i % 100000000 == 0) {
       fprintf(stdout, ".");
       fflush(stdout);
    }
  }
  fprintf(stdout, "\nreturn value: %f\n", ret);
  probe->stop();
  probe->print();
  fflush(stdout);
  return 0;
}


void sigusr2_handler(int sig)
{
    pid_t pid;
    fprintf(stdout, "child recieved signal from child pid is %d\n",getpid());
    probe->cont();
    probe->print();
    fflush(stdout);
}


