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
#include "msg_control.h"

#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
  unsigned long long num = atoi(argv[1]);
  double ret = 0.0;
  // set up the mqueue
  setup_mqueue();
  // set up the probe and start to detect;
  probe = new Probe();
  probe->init();
  probe->start();

  for (unsigned long long i = 0; i < num; i++) {
    ret *= (double) i;
    if (i % 100000000 == 0) {
       // send_state(id);
       // fprintf(stdout, ".");
       fflush(stdout);
    }
  }
  fprintf(stdout, "return value: %f\n", ret);
  probe->stop();
  probe->print();
  fflush(stdout);
  send_end();
  return 0;
}


