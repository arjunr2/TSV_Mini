#include <stdint.h>
#include "thread_common.h"

#define N 200000

uint64_t fibonacci[2] = {1, 0};
int num_ct = 1;
volatile int p = 0;

void *fib_thread(void *arg) {
  int tnum = *((int*)arg);
  int i = 0;
  while (1) {
    p += tnum;
    if (num_ct >= N) {
      break;
    }
    uint64_t f1 = fibonacci[1];
    fibonacci[1] = fibonacci[0];
    fibonacci[0] += f1;
    num_ct++;
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
    spawn_thread_tasks (fib_thread, NULL, true);
    result = fibonacci[0];
  }

  printf("Fib(%d) = %llu\n", N, result);
  printf("P = %d\n", p);

  return 0;
}
