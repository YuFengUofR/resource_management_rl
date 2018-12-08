#ifndef RL_CPU_FREQ_H
#define RL_CPU_FREQ_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


#define ERROR_MSG(msg) \
  { fprintf(stderr, "Error %s %s:line %d: \n", msg,__FILE__,__LINE__);  exit(-1); }

#define ROOT "/sys/devices/system/cpu/"
#define FREQ_PATH "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq"
#define GOV_PATH "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor"

static const char* const policy_string[] = {
  "conservative",
  "ondemand",
  "powersave",
  "performance"
};

enum Policy {
  kConservative,
  kOndemand,
  kPowersave,
  kPerformance,
  kNumPolicy,
  kInvalid = kNumPolicy,
};

typedef struct cpu_state_t * cpu_state;
struct cpu_state_t {
  int id;
  Policy policy;
  unsigned long freq;
};

class Controller {
  int num_cpu;
  cpu_state states;

 public:
  Controller() {
    // initialize the value of num_cpu;
    num_cpu = 0;
    DIR *dp;
    struct dirent *ep;
    // check how many cpu does machine have;
    dp = opendir(ROOT);
    if (dp != NULL)
    {
      while (ep = readdir(dp)) {
        if (strlen(ep->d_name) == 4 && (strncmp(ep->d_name,"cpu", 3)==0)) {
           // printf("this is cpu[%d]\n", atoi(&((ep->d_name)[3])));
           num_cpu += 1;
        }
      }
      closedir(dp);
    }
    else
      perror ("Couldn't open the directory");

    states = (cpu_state) calloc(num_cpu, sizeof(struct cpu_state_t));
    for (int i = 0; i < num_cpu; i++) {
      states[i].id = i;
      states[i].freq = 0lu;
    }
  }
  ~Controller(){}

  unsigned long get_cpu_freq(int id) {
    char file_path[50];
    FILE *fp;
    unsigned long ret_freq = 0;
    if (sprintf(file_path, FREQ_PATH, id) < 0) {
      ERROR_MSG("can't generate the cpu path");
    }
    // printf("PATH: %s\n", file_path);
    if ((fp = fopen(file_path, "r")) == NULL) {
      ERROR_MSG("can't open cpu_freq file.");
    } else {
      if (!fscanf(fp, "%lu", &ret_freq)) {
        ERROR_MSG("can't read cpu_freq from file.");
      }
      fclose(fp);
    }
    states[id].freq = ret_freq;
    return ret_freq;
  }

  int convert_cpu_policy_to_enum(const char * str) {
    int ret_policy = kInvalid;
    for (int i = 0; i < kNumPolicy; i++) {
      if (!strcmp(str, policy_string[i])) {
        ret_policy = i;
      }
    }
    return ret_policy;
  }

  int get_cpu_policy(int id) {
    char file_path[50];
    char ret_str[15];
    FILE *fp;
    int ret_policy = kInvalid;
    if (sprintf(file_path, GOV_PATH, id) < 0) {
      ERROR_MSG("can't generate the cpu path");
    }
    // printf("PATH: %s\n", file_path);
    if ((fp = fopen(file_path, "r")) == NULL) {
      ERROR_MSG("can't open cpu_freq file.");
    } else {
      if (!fscanf(fp, "%s", ret_str)) {
        ERROR_MSG("can't read cpu_freq from file.");
      }
      fclose(fp);
    }
    ret_policy = convert_cpu_policy_to_enum(ret_str);
    return ret_policy;
  }

  void change_cpu_policy(int id, int policy) {
    char cmd[48];
    char num[4];
    char str[12];

    if (sprintf(cmd, "sudo cpufreq-set -c %d -g %s", id, policy_string[policy]) < 0) {
      ERROR_MSG("can't generate the command");
    }
    printf("CMD: %s\n", cmd);
    if (system(cmd) == -1) {
      ERROR_MSG("can't execute the command.");
    }
  }
  const int get_num_cpu() {
    return num_cpu;
  }
};

#endif
