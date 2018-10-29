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
// #include "msg_queue.h"

#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>

struct msg_buffer {
   long mtype;
   char mtext[200];
};

Probe* probe;
int msqid;

void sigusr2_handler(int sig);
void tmp(mqd_t);

/* handle msg */
mqd_t connect_msg_queue(char);


int main(int argc, char **argv)
{
  unsigned long long num = atoi(argv[1]);
  double ret = 0.0;

  mqd_t id = connect_msg_queue('Q');

  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, sigusr2_handler);
  sigset_t ourmask;
  probe = new Probe();
  probe->init();
  probe->start();

  for (unsigned long long i = 0; i < num; i++) {
    ret *= (double) i;
    if (i % 100000000 == 0) {
       tmp(id);
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


mqd_t connect_msg_queue(char k) {
  key_t key;
  mqd_t id;
  char path[100];
  char *dir_path = getenv("HOME");

  if (!dir_path)
    return -1;

  strcpy(path, dir_path);
  strcat(path, "/msg_queue");

  if ((id = mq_open("/rlmqueue", O_WRONLY)) == -1) {
    perror("ERROR::on mq_open");
    exit(-1);
  }
  return id;
}


void sigusr2_handler(int sig)
{
  pid_t pid;
  int len;
  struct msg_buffer buf;
  fprintf(stdout, "child recieved signal from child pid is %d\n",getpid());
  probe->cont();
  probe->print();
  fflush(stdout);
  const struct counter_t vals = probe->get_state();
  sprintf(buf.mtext, "Stat %lld %lld %lld %lld %lld %lld %lld %lld",
       vals.total_instruction, vals.total_cycle,
       vals.l1_cache_miss, vals.l2_cache_miss, vals.l3_cache_miss,
       vals.branch_misprediction, vals.branch_instruction, vals.tlb_instruction_miss);
  len = strlen(buf.mtext);
  printf("sending msg: %s\n", buf.mtext);
  if (msgsnd(msqid, &buf, len+1, 0) == -1) {
    perror("ERROR:: on msgsnd");
  }
  kill(0, SIGUSR1);
}


void tmp(mqd_t id)
{
  pid_t pid;
  int len;
  char buf[200];
  fprintf(stdout, "From child pid is %d\n",getpid());
  probe->cont();
  // probe->print();
  // fflush(stdout);
  const struct counter_t vals = probe->get_state();
  sprintf(buf, "Stat %lld %lld %lld %lld %lld %lld %lld %lld",
       vals.total_instruction, vals.total_cycle,
       vals.l1_cache_miss, vals.l2_cache_miss, vals.l3_cache_miss,
       vals.branch_misprediction, vals.branch_instruction, vals.tlb_instruction_miss);
  len = strlen(buf);
  buf[len] = '\0';
  printf("sending msg: %s\n", buf);
  if (mq_send(id, buf, len+1, 0) != 0) {
    perror("ERROR:: on msgsnd");
  }
}
