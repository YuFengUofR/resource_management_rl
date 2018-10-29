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


/* self-defined interface */
#include "cpufreq.h"
#include "probe.h"

#define MAXLINE 100
#define MAXARGV 10

#define QUEUE_NAME  "/test_queue"

/* Function prototypes */
void eval(int, char **, char *);
int split_cmd(const char *, char *);
int builtin(const char *);
int parse_cmd(const char *);
ssize_t sio_puts(char *);

/* helper function */
key_t make_msg_queue(char);
void read_msg(int);

/* signal handlers */
void sigchld_handler(int sig);
void sigusr1_handler(int sig);
void sigusr2_handler(int sig);

// global variables
const char * prompt = "~Sh>>";
Controller *controller;
std::vector<pid_t> pid_q;

struct msg_buffer {
  long mtype;
  char msg[200];
};

typedef struct th_msg_t *th_msg;
struct th_msg_t {
  int cpu;
  int argc;
  char** argv;
  char** envp;
};

int main(int argc, char **argv) {
  int num_cpus = sysconf( _SC_NPROCESSORS_ONLN );

  char c;
  int index = 0, msqid = -1;
  uint64_t time_diff = 0L;
  struct timespec prev, curr;
  char cmdline[MAXLINE];  // Cmdline for fgets
  char program_line[MAXLINE];  // program command for execution
  bool emit_prompt = true;    // Emit prompt (default)

  // set the signal handler;
  signal(SIGCHLD, sigchld_handler);
  signal(SIGUSR2, SIG_IGN);
  signal(SIGUSR1, sigusr1_handler);

  // set the timer;
  clock_gettime(CLOCK_MONOTONIC, &prev);
  // make a msg queue;
  msqid = make_msg_queue('Q');

  printf("The usage of this program:\nPlease enter: \'<PROGRAM>\' <CPU> <POLICY>\n");

  controller = new Controller();

// Execute the shell's read/eval loop
  while (true)
  {
    if (emit_prompt)
    {
      fprintf(stdout, "%s", prompt);
      fflush(stdout);
    }
    usleep(10);
    // read_msg(msqid);
    // kill(0, SIGUSR2);
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
      return 0;
    }
    // Remove the trailing newline
    cmdline[strlen(cmdline)-1] = '\0';

    index = split_cmd(cmdline, program_line);
    if (index < 0) {
      printf("can't parse this line.\n");
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
    clock_gettime(CLOCK_MONOTONIC, &curr);
    int64_t diff = 1000000000 * (curr.tv_sec - prev.tv_sec) + curr.tv_nsec - prev.tv_nsec;
    if (diff >= 100000000) {
      kill(pid_q[0], SIGUSR2);
      prev.tv_sec = curr.tv_sec;
      prev.tv_nsec = curr.tv_nsec;
    }
    printf("SEND signal ..\n");
    kill(pid_q[0], SIGUSR2);
  }
  return -1; // control never reaches here
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


void read_msg(int id) {
  struct msg_buffer buf;
  if (msgrcv(id, &buf, sizeof(buf.msg), 0, 0) == -1) {
    perror("ERROR: can't read the msg");
  }
  printf("recvd: \"%s\"\n", buf.msg);
}


int  make_msg_queue(char k) {
  char path[100];
  key_t key;
  int msqid = -1;
  FILE * fd = NULL;
  char *dir_path = getenv("HOME");
  if (!dir_path)
    return -1;

  printf("haha %s\n", dir_path);
  strcpy(path, dir_path);
  strcat(path, "/msg_queue");
  fd = fopen(path, "w+");
  fclose(fd);
  if ((key = ftok(path, k)) == -1) {
    perror("ERROR::set the msg queue failed");
    exit(-1);
  }
  // attempt to create the IPC msg queue;
  if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1) {
    perror("ERROR:: msgget not working");
    exit(-1);
  }
  return msqid;
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

  sigset_t ourmask;
  sigset_t prev_mask;    // save previous mask;

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
    } else {
      pid_q.push_back(pid);
      printf("PID: %d", pid_q[0]);
    }
  }
}

ssize_t sio_puts(char *s) /* Put string */
{
    return write(STDOUT_FILENO, s, strlen(s));
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


void sigusr2_handler(int sig)
{
    pid_t pid;
    fprintf(stdout, "parent recieved signal...pid is %d\n", getpid());
}

void sigusr1_handler(int sig)
{
  struct msg_buffer buf;
  if (msgrcv(1, &buf, sizeof(buf.msg), 0, 0) == -1) {
    perror("ERROR: can't read the msg");
  }
  printf("recvd: \"%s\"\n", buf.msg);
}
