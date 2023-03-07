#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define NUM_THREADS 2

static void create_tasks_unsafe(pthread_t *tid, int num_threads, void *(*fn)(void*), 
    void* args, bool tid_arg) {
  for (int i = 0; i < num_threads; i++) {
    if (tid_arg) { args = &i; }
    if (pthread_create(&tid[i], NULL, fn, args)) {
      printf("Failed to create thread\n");
      exit(1);
    }
  }
}

static void create_tasks_safe(pthread_t *tid, int num_threads, void *(*fn)(void*), 
    void* args, bool tid_arg) {
  int ids[NUM_THREADS];
  for (int i = 0; i < num_threads; i++) {
    ids[i] = i;
    if (tid_arg) { args = &ids[i]; }
    if (pthread_create(&tid[i], NULL, fn, args)) {
      printf("Failed to create thread\n");
      exit(1);
    }
  }
}

static void join_tasks(pthread_t *tid, int num_threads) {
  for (int i = 0; i < num_threads; i++) {
    if (pthread_join(tid[i], NULL)) {
      printf("Failed to join thread\n");
      exit(1);
    }
  }
}

static void spawn_thread_tasks(void *(*fn)(void*), void* args, bool tid_arg) {
  pthread_t tid[NUM_THREADS];
  create_tasks_safe (tid, NUM_THREADS, fn, args, tid_arg);
  join_tasks (tid, NUM_THREADS);
}



static void init_mutex(pthread_mutex_t *lock) {
  if (pthread_mutex_init(lock, NULL) != 0) {
    printf("Mutex init failed\n");
    exit(1);
  }
}

static void destroy_mutex(pthread_mutex_t *lock) {
  if (pthread_mutex_destroy(lock) != 0) {
    printf("Mutex destroy failed\n");
    exit(1);
  }
}
