//  g++ main2.cc probe.h cpufreq.h -L/usr/local/lib -lpapi -lpthread -o main2
#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>          /* For O_* constants */
#include <sys/stat.h>       /* For mode constants */
#include <sys/ipc.h>		/* For IPC */
#include <sys/msg.h>		/* To send a msg */
#include <mqueue.h>
#include <time.h>
#include <stdint.h>

#include <fcntl.h>
#include <mqueue.h>

/* self-defined interface */
#include "cpufreq.h"
#include "probe.h"

#define QUEUE_NAME  "/rl_queue"
#define MAXLINE 	100
#define MAXARGV 	10
#define MAX_SIZE    200

/* some debug switch */

#ifndef DEBUG

#define DEBUG		false
#define CONTROL		false

#endif

/* Function helper */
int split_cmd(const char *, char *);
int builtin(const char *);
int parse_cmd(const char *);
ssize_t sio_puts(char *);

/* Function prototypes */
void eval(int, char **, char *);
void execute(char **);

/* interative console for debug */
void interactive_console(mqd_t);
void mad_runtime(mqd_t);

/* helper function */
mqd_t make_mqueue();
void read_mq(mqd_t);
void handle_msg(char *buf);
/* close up the mqueue;*/
void close_mqueue(mqd_t id);

/* signal handlers */
void sigchld_handler(int sig);
void sigint_handler(int sig);

// global variables
const char * prompt = "~Sh>>";
Controller *controller;
std::vector<pid_t> pid_q;
bool close_signal;
struct timespec prev, curr;

// thread structure might be useful;
typedef struct th_msg_t *th_msg;
struct th_msg_t {
  int cpu;
  int argc;
  char** argv;
  char** envp;
};

int main(int argc, char **argv) {
  int num_cpus = sysconf( _SC_NPROCESSORS_ONLN );
  close_signal = false;
  mqd_t msqid = -1;
  uint64_t time_diff = 0L;

  // set the signal handler;
  signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, sigint_handler);

  system("rm -rf /dev/mqueue/rl_queue");
  // set the timer;
  clock_gettime(CLOCK_MONOTONIC, &prev);
  clock_gettime(CLOCK_MONOTONIC, &curr);
  // make a msg queue;
  mqd_t id = make_mqueue();
  printf("The usage of this program:\nPlease enter: \'<PROGRAM>\' <CPU> <POLICY>\n");
  // create the controler to set cpufreq
  controller = new Controller();

  // start simulation;
  if (DEBUG) {
    interactive_console(id);
  } else {
    mad_runtime(id);
  }

  // close the mqueue whne finished;
  close_mqueue(id);
  printf("bye!\n");
  fflush(stdout);
  return -1; // control never reaches here
}

/* for interactive debug */
void interactive_console(mqd_t id) {
  char cmdline[MAXLINE];  // Cmdline for fgets
  char program_line[MAXLINE];  // program command for execution
  bool emit_prompt = true;    // Emit prompt (default)
  int index = 0;

  // Execute the shell's read/eval loop
  while (!close_signal)
  {
    if (emit_prompt)
    {
      fprintf(stdout, "%s", prompt);
      fflush(stdout);
    }
    usleep(10);
    read_mq(id);
    for (pid_t pid : pid_q) {
      if (DEBUG)
        printf("Send the signal to %d\n", pid);
      kill(pid, SIGUSR2);
    }
    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
    {
      fprintf(stderr, "ERROR: can't read the command properly.\n");
      exit(-1);
    }

    if (feof(stdin))
    {
    // End of file (ctrl-d)
      fprintf (stdout, "\n");
      fflush(stdout);
      fflush(stderr);
      return;
    }
    // Remove the trailing newline
    cmdline[strlen(cmdline)-1] = '\0';

    index = split_cmd(cmdline, program_line);
    if (index < 0) {
      // printf("can't parse this line.\n");
      continue;
    }
    char *token = strtok(program_line, " ");
    char **prog_argv = (char **) calloc(MAXARGV, sizeof(char *));
    int prog_argc = 0;

    while (token != NULL) {
      prog_argv[prog_argc] = (char *) calloc(strlen(token), sizeof(char));
      strcpy(prog_argv[prog_argc], token);
      // printf("%s\n", prog_argv[prog_argc]);
      prog_argc++;
      token = strtok(NULL, " ");
    };

    prog_argv[prog_argc] = (char *)0;
    fprintf(stdout, "the program argc is %d, argv: %s\n", prog_argc, program_line);

    // Evaluate the command line
    eval(prog_argc, prog_argv, &(cmdline[index]));
    fflush(stdout);
    fflush(stderr);
    usleep(10);

    // get the time and decide if need to set a signal;
    // clock_gettime(CLOCK_MONOTONIC, &curr);
    int64_t diff = 1000000000 * (curr.tv_sec - prev.tv_sec) + curr.tv_nsec - prev.tv_nsec;

    if (diff >= 1000000000) {
      prev.tv_sec = curr.tv_sec;
      prev.tv_nsec = curr.tv_nsec;
    }
    // free all the args memory;
    for (int i = 0; i < prog_argc; i++) {
      free(prog_argv[i]);
    }
    free(prog_argv);
  }
}

/* for interactive debug */
void mad_runtime(mqd_t id) {
  char cmdline[MAXLINE];  // Cmdline for fgets
  char program_line[MAXLINE];  // program command
  char* prog_argv[MAXARGV];
  struct timespec time;
  int index = 0;

  // Execute the shell's read/eval loop
  while (!close_signal)
  {
    // sleep a short time
    usleep(500000);
    unsigned long freq = controller->get_cpu_freq(0);
    clock_gettime(CLOCK_MONOTONIC, &time);
    fprintf(stdout, "[RL][%ld] Freq: %lu\n", (1000000000*time.tv_sec+time.tv_nsec)/1000, freq);
    read_mq(id);

    for (pid_t pid : pid_q) {
      // if (DEBUG)
        printf("Send the signal to %d\n", pid);
      kill(pid, SIGUSR2);
    }
    int cpu_id = 0;
    int random = rand() % 6;
    // "dedup", "blackscholes", "ferret", "freqmine", "streamcluster" ""
    switch (random) {
      case 0:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p facesim -i simlarge", cpu_id, "ondemand");
        break;
      case 1:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p blackscholes -i simlarge", cpu_id, "ondemand");
        break;
      case 2:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p ferret -i simlarge", cpu_id, "ondemand");
        break;
      case 3:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p freqmine -i simlarge", cpu_id, "ondemand");
        break;
      case 4:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p streamcluster -i simlarge", cpu_id, "ondemand");
        break;
      case 5:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p swaptions -i simlarge", cpu_id, "ondemand");
        break;
      default:
        sprintf(cmdline, "'%s' %d %s", "/home/tigris/parsec-3.0/bin/parsecmgmt -a run -p streamcluster -i simlarge", cpu_id, "ondemand");
    }
    // "dedup", "blackscholes", "ferret", "freqmine", "streamcluster"
    // Remove the trailing newline
    cmdline[strlen(cmdline)] = '\0';

	// find the rest of the code;
    index = split_cmd(cmdline, program_line);
    char *token = strtok(program_line, " ");
    int prog_argc = 0;

    while (token != NULL) {
      prog_argv[prog_argc] = (char *) calloc(strlen(token), sizeof(char));
      strcpy(prog_argv[prog_argc], token);
      // printf("%s\n", prog_argv[prog_argc]);
      prog_argc++;
      token = strtok(NULL, " ");
    };
    // end the program argv
    prog_argv[prog_argc] = NULL;
    if (DEBUG)
      fprintf(stdout, "the program argc is %d, argv: %s\n", prog_argc, program_line);
    // get the time and decide if need to set a signal;
    int64_t pivot = rand() % 10 + 5;
    clock_gettime(CLOCK_MONOTONIC, &curr);
    int64_t diff = 1000000000 * (curr.tv_sec - prev.tv_sec) + curr.tv_nsec - prev.tv_nsec;
    if (diff > (pivot * (int64_t) 1000000000)) {
      // swap the time value
      printf("[RL][%ld] program type %d\n",(1000000000*curr.tv_sec+curr.tv_nsec)/1000, random);
      printf("%d:%d\n", (int)diff/1000000000, (int)pivot);
      prev.tv_sec = curr.tv_sec;
      prev.tv_nsec = curr.tv_nsec;
      // Evaluate the command line
      execute(prog_argv);
    }
    fflush(stdout);
    fflush(stderr);
    usleep(10);

    // free all the args memory;
    for (int i = 0; i < prog_argc; i++) {
      free(prog_argv[i]);
    }
  }
}


int split_cmd(const char *cmdline, char *program_line) {
  char program_cmd[MAXLINE];
  int len = strlen(cmdline);
  bool start_to_copy = false;
  int cnt = 0;
  int index = -1;
  for (int i = 0; i < len; i++) {
    if (cmdline[i] == '\'' && !start_to_copy) {
      start_to_copy = true;
    } else if (start_to_copy && cmdline[i] != '\'') {
      program_cmd[cnt++] = cmdline[i]; 
    } else if (start_to_copy && cmdline[i] == '\'') {
      index = i;
      program_cmd[cnt] = '\0';
      break;
    }
  }
  if (index == -1) {
    program_line = NULL;
    return -1;
  } else {
    strcpy(program_line, program_cmd);
    return index+1;
  }
}

/* handle messages and open/close the mqueue */
void read_mq(mqd_t mq) {
  char buf[MAX_SIZE + 1];
  int bytes_read = 0;
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  while (true) {
    memset(buf, 0x00, sizeof(buf));
    int bytes_read = mq_receive(mq, buf, MAX_SIZE, NULL);
    if(bytes_read >= 0) {
      printf("[RL][%ld] Received: %s\n",
              (1000000000*time.tv_sec+time.tv_nsec)/1000, buf);
      handle_msg(buf);
      fflush(stdout);
    } else {
      break;
    }
  }
}

void handle_msg(char *buf) {
  pid_t pid = -1;
  int cnt = 0;
  long long val[8];
  if (buf[0] == 'P') {
    cnt = sscanf(buf, "PID %d", &pid);
    if (cnt > 0)
    	pid_q.push_back(pid);

  } else if (buf[0] == 'S'){
    sscanf(buf, "STAT %d %lld %lld %lld %lld %lld %lld %lld %lld ",
        &pid, &(val[0]), &(val[1]), &(val[2]), &(val[3]),
              &(val[4]), &(val[5]), &(val[6]), &(val[7]));
    // TODO: save the data
  } else if (buf[0] == 'E'){
    cnt = sscanf(buf, "END %d", &pid);
    if (cnt > 0) {
      for (int i = 0; i < pid_q.size(); i++) {
        if (pid_q[i] == pid) {
          pid_q.erase(pid_q.begin()+i);
          break;
        }
      }
    }
  } else {
    perror("Wrong message");
  }
}


mqd_t make_mqueue() {
  mqd_t mq;
  ssize_t bytes_read;
  struct mq_attr attr;
  char buffer[MAX_SIZE + 1];

  /* initialize the queue attributes */
  attr.mq_flags = 0;
  attr.mq_maxmsg = 100;
  attr.mq_msgsize = MAX_SIZE;
  attr.mq_curmsgs = 0;

  /* create the message queue */
  mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0644, &attr);
  if (mq == -1)
    ERROR_MSG("Building mqueue failed.\n");
  return mq;
}

void close_mqueue(mqd_t id) {
  if(mq_close(id) == -1) {
    perror ("ERROR::mq_close");
    exit (-1);
  }
  if(mq_unlink(QUEUE_NAME) == -1) {
    perror ("ERROR::mq_unlink");
    exit (-1);
  }
}


void eval(int prog_argc, char **prog_argv, char *cmdline)
{
  int policy = kInvalid, cpu;
  char program_name[MAXLINE];
  char policy_name[MAXLINE];

  // fprintf(stdout, "left cmd: %s\n", cmdline);
  // Parse command line
  if (sscanf(cmdline, "%d %s", &cpu, policy_name) !=  2) {
    ERROR_MSG("Input command is invalid.\n");
  }

  for (int i = 0; i < kNumPolicy; i++) {
    if (strcmp(policy_name, policy_string[i]) == 0) {
      policy = i;
      break;
    }
  }

  if (policy == kInvalid || cpu >= controller->get_num_cpu()) {
    ERROR_MSG("current policy or cpu id is invalid.");
  }

  // check if built-in commands
  if (program_name != NULL) {
    pid_t pid;
    controller->change_cpu_policy(cpu, policy);

    if ((pid = fork()) == 0) {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(cpu, &cpuset);

      if (sched_setaffinity(getpid(), sizeof(cpuset), &cpuset) == -1)
        ERROR_MSG("set affinity failed");

      // switch back the mask;
      // sigprocmask(SIG_SETMASK, &prev_mask, NULL);

     // start the real function part;
      if (execvp(prog_argv[0], prog_argv) == -1) {
        ERROR_MSG("Execvp can't be run, ");
        fflush(stderr);
      }
    }
  }
}


void execute(char **prog_argv)
{
  pid_t pid;
  // controller->change_cpu_policy(cpu, policy);

  if ((pid = fork()) == 0) {
    // start the real function part;
    if (execvp(prog_argv[0], prog_argv) == -1) {
      ERROR_MSG("Execvp can't be run, ");
      fflush(stderr);
    }
  }
}


void sigchld_handler(int sig)
{
    pid_t pid;
    int status;
    /* reap the zombie "kids" */
    while ((pid = waitpid (-1, &status, WNOHANG|WUNTRACED)) > 0 )
    {
      continue;
    }
}

/*
 * sigint_handler: kill the process
 */
void sigint_handler(int sig)
{
  close_signal = true;
}
