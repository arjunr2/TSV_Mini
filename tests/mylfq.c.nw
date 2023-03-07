#include "thread_common.h"
#include <stdatomic.h>
#include <stdio.h>


typedef struct {
  int val;
} qelem;

typedef struct {
  int maxbits;
  atomic_int head;
  atomic_int tail;
  atomic_int write_index;
  atomic_int read_index;
  qelem *elems;
} qstate;



void qinit (qstate *state, qelem *elems, int ct) {
  state->maxbits = (1 << __builtin_ctz(ct)) - 1;
  state->head = 0;
  state->tail = 0;
  state->write_index = 0;
  state->read_index = 0;
  state->elems = elems;
}

inline int norm(qstate *state, int val) {
  return val & state->maxbits;
}

bool qenqueue (qstate *state, qelem elem, bool *wait) {
  *wait = false;
  // Reserve write_index for this thread
  int write_index = norm(state, atomic_fetch_add(&state->write_index, 1));
  // Wait till tail hits write_index to commit
  while (write_index != norm(state, atomic_load(&state->tail))) { *wait = true; };
  // If full, do not commit
  if (write_index == norm(state, atomic_load(&state->head) - 1)) { return false; }
  printf("Committing write to index %d\n", write_index);
  state->elems[write_index] = elem;
  atomic_fetch_add(&state->tail, 1);
  return true;
}


bool qdequeue (qstate *state, qelem* elem, bool *wait) {
  *wait = false;

  int read_index = norm(state, atomic_fetch_add(&state->read_index, 1));
  *elem = state->elems[read_index];
  // Wait till head hits read_index to commit 
  while (read_index != norm(state, atomic_load(&state->head))) { *wait = true; };
  // If tail == head when committing, cannot dequeue
  if (norm(state, atomic_load(&state->tail)) == read_index) { return false; }
  printf("Committing read to index %d\n", read_index);
  atomic_fetch_add(&state->head, 1);
  return true;
}

#define IT 200

void *rapid_enqueue_dequeue_thread(void *arg) {
  qstate *state = (qstate*) arg;
  qelem deq_elem;
  bool dequeue_wait, enqueue_wait;
  int dequeue_wait_ct = 0;
  int enqueue_wait_ct = 0;
  int dequeue_success_ct = 0;
  int enqueue_success_ct = 0;
  for (int i = 0; i < IT; i++) {
    qelem enq_elem = { .val = i };
    dequeue_success_ct += qdequeue (state, &deq_elem, &dequeue_wait);
    enqueue_success_ct += qenqueue (state, enq_elem, &enqueue_wait);
    dequeue_wait_ct += dequeue_wait;
    enqueue_wait_ct += enqueue_wait;
  }
  printf("Total %d | Enqueue -- S: %d, W: %d | Dequeue -- S: %d, W: %d\n", 
    IT, enqueue_success_ct, enqueue_wait_ct, dequeue_success_ct, dequeue_wait_ct);
  return NULL;
}

int main() {
  qelem elems[16];
  qstate state;
  qinit (&state, elems, 16);

  qelem elem = { .val = 0 };
  bool w;
  qenqueue (&state, elem, &w);

  pthread_t tid[3];
  create_tasks(tid, 3, rapid_enqueue_dequeue_thread, &state, false);
  join_tasks(tid, 3);
  printf("Bits: %d\n", state.maxbits);
  return 0;
}
