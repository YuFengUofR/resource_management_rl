#include "cpufreq.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

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
  fprintf(stdout, "DONE!\n");
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
