#include "cpufreq.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <cstdint>

#define MAXARGV 9
#define MAXTOKEN 20

// "swaptions" "facesim" "freqmine" "ferret" "fluidanimate" "blackscholes" "streamcluster" "dedup" "canneal"
const char *APP[] = {"swaptions", "facesim", "freqmine", "ferret", 
                     "fluidanimate", "blackscholes", "streamcluster", 
                     "dedup", "canneal"};

void sigchld_handler(int sig);
int wait_cnt;


int main(int argc, char** argv) {
  if (argc != 4) {
    fprintf(stderr, "USAGE: ./<self> <num> <sleep_time> <policy>");
    exit(-1);
  }
  // This program defines a single-stage imaging pipeline that
  // brightens an image.
  struct timespec start, end;
  uint64_t delta_ms = 0;

  char cmd[48];
  if (sprintf(cmd, "sudo cpufreq-set -r -g %s", argv[3]) < 0)
    ERROR_MSG("can't generate the command");

  printf("CMD: %s\n", cmd);
  if (system(cmd) == -1)
    ERROR_MSG("can't execute the command.");

  unsigned long long round = atoi(argv[1]);
  unsigned dtime = atoi(argv[2]);
  srand(0);
  signal(SIGCHLD, sigchld_handler);
  wait_cnt = 0;
  // parsecmgmt -a run -p netferret -i simlarge
  char **prog_argv = (char **) calloc(MAXARGV, sizeof(char *));

  for (int i = 0; i < MAXARGV; i++) {
    prog_argv[i] = (char *) calloc(MAXTOKEN, sizeof(char));
  }
  strcpy(prog_argv[0],"sudo");
  strcpy(prog_argv[1], "parsecmgmt");
  strcpy(prog_argv[2], "-a");
  strcpy(prog_argv[3], "run");
  strcpy(prog_argv[4], "-p");
  strcpy(prog_argv[6], "-i");
  strcpy(prog_argv[7], "simlarge");
  prog_argv[8] = (char *) 0;

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  pid_t pid;
  for (int i = 0; i < round; i++) {
    unsigned int num = (rand() % 9);
    strcpy(prog_argv[5], APP[num]);
    if ((pid = fork()) == 0) {
      if (execvp(prog_argv[0], prog_argv) == -1) {
        ERROR_MSG("Execvp can't be run, ");
        fflush(stderr);
      }
    }
    sleep(dtime);
  }
  while(true) {
    if (wait_cnt != round) {
      pause();
    } else {
      break;
    }
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  delta_ms += (end.tv_sec-start.tv_sec) * 1000
                        + (end.tv_nsec-start.tv_nsec) / 1000000;

  fprintf(stdout, "DONE!\n");
  fprintf(stdout, "[IMPORTANT_RESULT] Total run time : %lu ms.\n", delta_ms);
  return 0;
}

void sigchld_handler(int sig)
{
    pid_t pid;
    int status;
    /* reap the zombie "kids" */ 
    while ((pid = waitpid (-1, &status, WNOHANG|WUNTRACED)) > 0 ) 
    {
      wait_cnt++;
      fprintf(stdout, "\n\nPID: %d.\n\n", pid);
      continue;
    }
}
