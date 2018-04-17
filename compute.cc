#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
  unsigned long long num = atoi(argv[1]);
  double ret = 0.0;
  for (unsigned long long i = 0; i < num; i++) {
    ret *= (double) i;
    if (i % 100000000 == 0) {
       printf(".");
       fflush(stdout);
    }
  }
  printf("return value: %f\n", ret);
  return 0;
}
