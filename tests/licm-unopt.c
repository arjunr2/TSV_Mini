
#include "thread_common.h"

#define LOOP_CT 5000000

volatile int y = 3;
volatile int z = 4;
volatile int a = 0;

void *task_thread1(void *arg) {
  printf("Start1\n");
  int x;
  for (int i = 0; i < LOOP_CT; i++) {
    x = y + z;
    a += x * y;
  }
  printf("Done1\n");
  return NULL;
}

void *task_thread2(void *arg) {
  printf("Start2\n");
  for (int i = 0; i < LOOP_CT; i++) {
    y = 32;
  }
  printf("Done2\n");
  return NULL;
}

int main() {
  pthread_t tid[2];
  pthread_create(&tid[0], NULL, task_thread1, NULL);
  pthread_create(&tid[1], NULL, task_thread2, NULL);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  printf("a = %d\n", a);
  return 0;
}
