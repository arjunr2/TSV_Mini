#include <stdint.h>
#include "thread_common.h"

#define NUM_THREADS 2
#define N 200000

uint64_t fibonacci[2] = {1, 0};
int num_ct = 1;
volatile int p = 0;
pthread_mutex_t lock;

void *fib_thread(void *arg) {
  int tnum = *((int*)arg);
  int i = 0;
  while (1) {
    pthread_mutex_lock(&lock);
    p += tnum;
    if (num_ct >= N) { 
      pthread_mutex_unlock(&lock);
      break; 
    } 
    uint64_t f1 = fibonacci[1];
    fibonacci[1] = fibonacci[0];
    fibonacci[0] += f1;
    num_ct++;
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}


pthread_t tid[NUM_THREADS];
uint64_t result;

int main() {
  if (N == 0) {
    result = fibonacci[1];
  }
  else if (N == 1) {
    result = fibonacci[0];
  }
  else {
    init_mutex(&lock);
    /* Don't run spawn_thread_tasks to prove unsafe create */
    create_tasks_unsafe (tid, NUM_THREADS, fib_thread, NULL, true);
    join_tasks (tid, NUM_THREADS);
    destroy_mutex(&lock);
    result = fibonacci[0];
  }

  printf("Fib(%d) = %llu\n", N, result);
  printf("P = %d\n", p);

  return 0;
}
