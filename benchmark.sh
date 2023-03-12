#!/bin/bash

RES_DIR=results
mkdir -p $RES_DIR

# Run all instructions instrumented access
make access
./run_access.py all &> $RES_DIR/single_run.res

for run_no in {1..10}; do
  for batch_size in {2..10..2}; do
    for id in {10..90..10}; do
      make -B access-batch STOCH=$id BATCH_SIZE=$batch_size
      ./run_access.py --batch=$batch_size all &> $RES_DIR/run_${run_no}_${id}_${batch_size}.res
    done
  done
done

