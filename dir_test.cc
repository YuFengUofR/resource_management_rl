#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "cpufreq.h"


int main (void)
{
  Controller cpu_control;
  int num_cpu = cpu_control.get_num_cpu();
  for (int i = 0; i < num_cpu; i++) {
    printf("CPU[%d] has policy %d with freq %lu.\n", i, 
                            cpu_control.get_cpu_policy(i),
                            cpu_control.get_cpu_freq(i));
  }
  cpu_control.change_cpu_policy(1, 1);
  return 0;
}

