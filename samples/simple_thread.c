#include <pthread.h>
#include <stdio.h>

void *thread_routine(void *arg) {
  printf("Val is %d\n", *((int*)arg));
  return NULL;
}

int main() {
 pthread_t tid;
 int val = 42;
 if (pthread_create(&tid, NULL, thread_routine, &val)) {
   printf("Pthread create error");
 }
 pthread_join(tid, NULL);
 return 0;
}
