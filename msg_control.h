#ifndef RL_MSG_CONTROL_H
#define RL_MSG_CONTROL_H

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

#include <sys/msg.h>
#include <sys/ipc.h>
#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>

#define QUEUE_NAME	"/rl_queue"
#define DEBUG_SIG	true

/* some gloval variables */
Probe* probe;	// probe to retrieve cpu information
mqd_t msqid;	// meg queue id to identify the queue id;

/*
  set up the configuration;
  1. connect to the msg queue;
  2. set the signal handler;
  3. set the global msqid;
*/
void setup_mqueue(void);

/* signal handler */
void sigusr2_handler(int);
mqd_t connect_msg_queue(void);  /* handle msg */

/* send the information to the queue */
void send_state(mqd_t, Probe*);
void send_pid(mqd_t);
void send_end();

/* close up the queue on this end */


void setup_mqueue(void) {
  mqd_t id = connect_msg_queue();
  send_pid(id);
  msqid = id;
  probe = new Probe();
  probe->init();
  probe->start();
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, sigusr2_handler);
  if (DEBUG_SIG)
    printf("Setup the sig-handlers and msg-queue\n");
}

/* function implementation  */
void sigusr2_handler(int sig)
{
  pid_t pid;
  int len;
  char buf[200];
  fprintf(stdout, "child recieved signal from child pid is %d\n",getpid());
  probe->cont();
  if (DEBUG_SIG) {
    probe->print();
    fflush(stdout);
  }
  pid = getpid();
  // get he information from the probe;
  const struct counter_t vals = probe->get_state();
  // copy into a string;
  sprintf(buf, "Stat %d %lld %lld %lld %lld %lld %lld %lld %lld",
       pid, vals.total_instruction, vals.total_cycle,
       vals.l1_cache_miss, vals.l2_cache_miss, vals.l3_cache_miss,
       vals.branch_misprediction, vals.branch_instruction, vals.tlb_instruction_miss);
  len = strlen(buf);
  buf[len] = '\0';
  if (DEBUG_SIG) {
  	printf("sending msg: %s\n", buf);
    fflush(stdout);
  }
  // printf("msqid: %d\n", msqid);
  if (mq_send(msqid, buf, len+1, 0) != 0) {
    perror("ERROR::on mq_send");
  }
}

mqd_t connect_msg_queue() {
  mqd_t id;
  if ((id = mq_open(QUEUE_NAME, O_WRONLY)) == -1) {
    perror("ERROR::on mq_open");
    exit(-1);
  }
  return id;
}

void send_pid(mqd_t id) {
  pid_t pid;
  int len;
  char buf[20];
  pid = getpid();
  sprintf(buf, "PID %d", pid);
  if (DEBUG_SIG)
    fprintf(stdout, "From child pid is %d\n",getpid());
  len = strlen(buf);
  buf[len] = '\0';
  if (DEBUG_SIG)
    printf("sending msg: %s\n", buf);

  if (mq_send(id, buf, len+1, 0) != 0) {
    perror("ERROR::on msgsnd");
  }
}

void send_end() {
  pid_t pid;
  int len;
  char buf[20];
  pid = getpid();
  sprintf(buf, "END %d", pid);
  fprintf(stdout, "Close child pid is %d\n",getpid());
  len = strlen(buf);
  buf[len] = '\0';
  // printf("sending msg: %s\n", buf);
  if (mq_send(msqid, buf, len+1, 0) != 0) {
    perror("ERROR::on msgsnd");
  }
}

void send_state(mqd_t id, Probe* probe)
{
  pid_t pid;
  int len;
  char buf[200];
  // save for signal
  if (DEBUG_SIG)
    fprintf(stdout, "From child pid is %d\n",getpid());

  pid = getpid();
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
    perror("ERROR::on mq_send");
  }
}

#endif
