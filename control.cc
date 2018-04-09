#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#define _GNU_SOURCE
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

// The <errno.h> header file defines the integer variable errno, which is set by system calls and some library functions in the event of an error to indicate what went wrong.
#define print_error_then_terminate(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#define print_perror_then_terminate(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

  struct thread_info {

    pthread_t thread_id; // ID returned by pthread_create()
    int core_id; // Core ID we want this pthread to set its affinity to
  };

#define SUCCESS_MSG "Successfully set thread %lu to affinity to CPU %d\n"
#define FAILURE_MSG "Failed to set thread %lu to affinity to CPU %d\n"


void * thread_camper(void *arg) {

  struct thread_info *thread_info = arg;

  const pthread_t pid = pthread_self();
  const int core_id = thread_info->core_id;

  // cpu_set_t: This data set is a bitset where each bit represents a CPU.
  cpu_set_t cpuset;
  // CPU_ZERO: This macro initializes the CPU set set to be the empty set.
  CPU_ZERO(&cpuset);
  // CPU_SET: This macro adds cpu to the CPU set set.
  CPU_SET(core_id, &cpuset);

  // pthread_setaffinity_np: The pthread_setaffinity_np() function sets the CPU affinity mask of the thread thread to the CPU set pointed to by cpuset. If the call is successful, and the thread is not currently running on one of the CPUs in cpuset, then it is migrated to one of those CPUs.
  const int set_result = pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuset);
  if (set_result != 0) {

    print_error_then_terminate(set_result, "pthread_setaffinity_np");
  }

  // Check what is the actual affinity mask that was assigned to the thread.
  // pthread_getaffinity_np: The pthread_getaffinity_np() function returns the CPU affinity mask of the thread thread in the buffer pointed to by cpuset.
  const int get_affinity = pthread_getaffinity_np(pid, sizeof(cpu_set_t), &cpuset);
  if (get_affinity != 0) {

    print_error_then_terminate(get_affinity, "pthread_getaffinity_np");
  }

  char *buffer;
  // CPU_ISSET: This macro returns a nonzero value (true) if cpu is a member of the CPU set set, and zero (false) otherwise. 
  if (CPU_ISSET(core_id, &cpuset)) {

    const size_t needed = snprintf(NULL, 0, SUCCESS_MSG, pid, core_id);
    buffer = malloc(needed);
    snprintf(buffer, needed, SUCCESS_MSG, pid, core_id);
  } else {

    const size_t needed = snprintf(NULL, 0, FAILURE_MSG, pid, core_id);
    buffer = malloc(needed);
    snprintf(buffer, needed, FAILURE_MSG, pid, core_id);
  }

  return buffer;
}

int main(int argc, char *argv[]) {

  // Initialize thread creation attributes
  pthread_attr_t attr;
  const int attr_init_result = pthread_attr_init(&attr);
  if (attr_init_result != 0) {
      print_error_then_terminate(attr_init_result, "pthread_attr_init");
  }

  // We will set the stack size limit to is 1 MB (0x100000 bytes)
  const int stack_size = 0x100000;
  const int setstacksize_result = pthread_attr_setstacksize(&attr, stack_size);
  if (setstacksize_result != 0) {
    print_error_then_terminate(setstacksize_result, "pthread_attr_setstacksize");
  }

  const int num_threads = 4;
  // Allocate memory for pthread_create() arguments
  struct thread_info *thread_info = calloc(num_threads, sizeof(struct thread_info));
  if (thread_info == NULL) {
      print_perror_then_terminate("calloc");
  }


  // Create the threads and initialize the core_id argument, which will be used to set the thread to the specific CPU core.
  // For example, we want the first pthread to camp on the first CPU core which has the ID 0. So we pass the value 0 to its core_id.
  int tnum;
  for (tnum = 0; tnum < num_threads; tnum++) {

    thread_info[tnum].core_id = tnum;
    // The pthread_create() call stores the thread ID into corresponding element of thread_info[]
    const int create_result = pthread_create(&thread_info[tnum].thread_id, &attr, &thread_camper, &thread_info[tnum]);
    if (create_result != 0) {
      print_error_then_terminate(create_result, "pthread_create");
    }
  }

  // Destroy the thread attributes object, since it is no longer needed
  const int destroy_result = pthread_attr_destroy(&attr);
  if (destroy_result != 0) {
   
   print_error_then_terminate(destroy_result, "pthread_attr_destroy");
  }

  // Now join with each thread, and display its returned value
  for (tnum = 0; tnum < num_threads; tnum++) {
    void *res;
    const int join_result = pthread_join(thread_info[tnum].thread_id, &res);
    if (join_result != 0) {
      print_error_then_terminate(join_result, "pthread_join");
    }

    printf("Joined with thread %d; returned value was %s\n", thread_info[tnum].core_id, (char *) res);
    free(res); // Free memory allocated by thread
  }

  free(thread_info);
  return 0;
}
