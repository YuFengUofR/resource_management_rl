#include <stdio.h>
#include <stdlib.h>
#include "papi.h"


#define ERROR_INT(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__);  exit(retval); }

typedef struct counter_t *cpu_info;
struct counter_t {
  long long total_instruction;
  long long total_cycle;
  long long l1_cache_miss;
  long long l2_cache_miss;
  long long l3_cache_miss;
  long long branch_misprediction;
  long long branch_instruction;
  long long tlb_instruction_miss;
};

char errstring[PAPI_MAX_STR_LEN];
int NUM_EVENTS = 8;
int Events[8] = {PAPI_TOT_INS, PAPI_TOT_CYC, 
                  PAPI_L1_TCM, PAPI_L2_TCM, PAPI_L3_TCM,
                  PAPI_BR_MSP, PAPI_BR_INS,
                  PAPI_TLB_IM};

class Probe {
  long long *retvals; 
  cpu_info state;
  int retval;
  int num_hwcntrs;

 public:
  Probe() {
    state = (cpu_info) calloc(1, sizeof(struct counter_t));
    retvals =  (long long *) calloc(NUM_EVENTS, sizeof(long long));
  }
  ~Probe() {
    delete state;
  }
  int init() {
    if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT ) {
      fprintf(stderr, "Error: %d %s\n",retval, errstring);
      exit(-1);
   }
   if ((num_hwcntrs = PAPI_num_counters()) < PAPI_OK) {
      printf("There are no counters available. \n");
      exit(-1);
   }
   // printf("There are %d counters in this system\n",num_hwcntrs);
   if (num_hwcntrs < NUM_EVENTS) {
      printf("Not enough counters support on the device.\n");
      exit(-1);
   }
   
  }
  void start() {
    if ( (retval = PAPI_start_counters(Events, NUM_EVENTS)) != PAPI_OK)
       ERROR_INT(retval);
  }
  void stop() {
    if ((retval=PAPI_stop_counters(retvals, NUM_EVENTS)) != PAPI_OK)
      ERROR_INT(retval); 
  }
  void cont() {
    if ( (retval=PAPI_accum_counters(retvals, NUM_EVENTS)) != PAPI_OK)
      ERROR_INT(retval);
  }

  const struct counter_t& get_state() {
    state->total_instruction = retvals[0];
    state->total_cycle = retvals[1];
    state->l1_cache_miss = retvals[2];
    state->l2_cache_miss = retvals[3];
    state->l3_cache_miss = retvals[4];
    state->branch_misprediction = retvals[5];
    state->branch_instruction = retvals[6];
    state->tlb_instruction_miss = retvals[7];
    return *state;
  }

  void print() {
    fprintf(stdout, "The total instructions are %lld and total cycles used are %lld.\n",
            retvals[0], retvals[1]);
    fprintf(stdout, "The total cache misss in L1, L2, L3 are %lld, %lld, %lld.\n",
            retvals[2], retvals[3], retvals[4]);
    fprintf(stdout, "The total branch mis-prediction and instructions are %lld, %lld.\n",
            retvals[5], retvals[6]);
    fprintf(stdout, "The total tlb_instruction_miss are %lld.\n", retvals[7]);
  }
};


