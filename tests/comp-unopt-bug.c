
#include "thread_common.h"

#define LOOP_CT 5000000

int y = 3;
int z = 4;
volatile int a = 0;

void *task_thread1(void *arg) {
  printf("Start1\n");
  for (int i = 0; i < LOOP_CT; i++) {
    a += y * z;
  }
  printf("Done1\n");
  return NULL;
}

void *task_thread2(void *arg) {
  printf("Start2\n");
  int l = 0;
  for (int i = 0; i < 5 * LOOP_CT; i++) {
    l += 1;
    y = 32;
  }
  printf("Done2\n");
  return NULL;
}

/* Program intent is to compute a = LOOP_CT * (y^2 + yz) */
/* Perfomed in Thread1, and Thread2 is an interfering data-race task 
* LICM optimization for Thread2 makes it very likely to see the y data race */
int main() {
  pthread_t tid[2];
  pthread_create(&tid[0], NULL, task_thread1, NULL);
  pthread_create(&tid[1], NULL, task_thread2, NULL);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  printf("a = %d\n", a);
  return 0;
}
