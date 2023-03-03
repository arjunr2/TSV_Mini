#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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
    if (pthread_mutex_init(&lock, NULL) != 0) {
      printf("Mutex init failed\n");
      exit(1);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
      if (pthread_create(&tid[i], NULL, fib_thread, &i)) {
        printf("Failed to create thread\n");
        exit(1);
      }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
      if (pthread_join(tid[i], NULL)) {
        printf("Failed to join thread\n");
        exit(1);
      }
    }
    pthread_mutex_destroy(&lock);
    //printf("Joined threads successfully!\n");
    result = fibonacci[0];
  }

  printf("Fib(%d) = %llu\n", N, result);
  printf("P = %d\n", p);

  return 0;
}
