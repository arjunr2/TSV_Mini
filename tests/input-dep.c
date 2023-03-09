/*
!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!!
!!! Copyright (c) 2017-20, Lawrence Livermore National Security, LLC
!!! and DataRaceBench project contributors. See the DataRaceBench/COPYRIGHT file
for details.
!!!
!!! SPDX-License-Identifier: (BSD-3-Clause)
!!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!!
 */

/* 
 * Input dependence race: example from OMPRacer: A Scalable and Precise Static Race
   Detector for OpenMP Programs
 * Data Race Pair, A[0]@45:7:W vs. A[i]@42:5:W
 * */

#include <stdio.h>
#include <stdlib.h>
#include "thread_common.h"

#define N 10001
#define THRESHOLD 10000

int A[N];

void load_from_input(int *data, int size)
{
  for(int i = 0; i < size; i++) {
    data[i] = size-i;
  } 
}


void *task_thread(void *arg) {
  int id = *((int*)arg);
  for (int j = 0; j < 100; j++) {
    for (int i = id; i < N; i += NUM_THREADS) {
      A[i] = i;
      if (N > THRESHOLD) {
        A[0] = 1;
      }
    }
  }
  return NULL;
}

int main() {

  load_from_input(A, N);
  
  spawn_thread_tasks(task_thread, NULL, true);

  return 0;
}
