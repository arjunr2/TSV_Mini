#include <stdio.h>
#include <string.h>
#include "thread_common.h"
#include "liblfds711.h"

#define NUM_THREADS 2
#define QSIZE (1 << 6)

#define NUM_ITERATIONS 20000

struct test_data {
  char name[64];
};


void* fast_enqueue_dequeue_thread(void* arg) {

  struct lfds711_queue_bmm_state* qbmms = (struct lfds711_queue_bmm_state*) arg;

  LFDS711_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  
  uint32_t *dequeue_val;
  uint32_t total_enqueues = 0;
  int rv = 0;
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    lfds711_queue_bmm_dequeue ( qbmms, NULL, &dequeue_val);
    rv = lfds711_queue_bmm_enqueue( qbmms, NULL, (void*) &i );
    if (rv == 1) { total_enqueues++; }
  }
  //printf("Total enqueues: %u\n", total_enqueues);
  return NULL;
}




int main()
{
  struct lfds711_queue_bmm_element qbmme[QSIZE]; // TRD : must be a positive integer power of 2 (2, 4, 8, 16, etc)

  struct lfds711_queue_bmm_state qbmms;

  struct test_data td, *temp_td;  

  lfds711_queue_bmm_init_valid_on_current_logical_core( &qbmms, qbmme, QSIZE, NULL );
  for (int i = 0; i < QSIZE; i++) {
    lfds711_queue_bmm_enqueue ( &qbmms, NULL, NULL );
  }

  pthread_t tid[NUM_THREADS];
  create_tasks  (tid, NUM_THREADS, fast_enqueue_dequeue_thread, (void*) &qbmms, false);
  join_tasks (tid, NUM_THREADS);

  //strcpy( td.name, "Madge The Skutter" );

  //lfds711_queue_bmm_enqueue( &qbmms, NULL, &td );

  //lfds711_queue_bmm_dequeue( &qbmms, NULL, &temp_td );

  //printf( "skutter name = %s\n", temp_td->name );

  lfds711_queue_bmm_cleanup( &qbmms, NULL );
  printf("Done!\n");

  return 0;
}
