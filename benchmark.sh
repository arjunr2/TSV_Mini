#!/bin/bash

RES_DIR=results
mkdir -p $RES_DIR

for run_no in {1..20}; do
  for batch_size in {2..8..2}; do
    make
    make access
    make -B access-batch BATCH_SIZE=$batch_size

    sleep 4

    # Run no instrumentation 
    ./run_tests.py --mode=normal all &> $RES_DIR/noinst_run_${run_no}_${batch_size}.res
    # Run all instructions instrumented access
    ./run_tests.py --mode=access all &> $RES_DIR/allinst_run_${run_no}_${batch_size}.res
    # Run sharded instrumentation access
    ./run_tests.py --mode=access --batch=$batch_size all &> $RES_DIR/run_${run_no}_${batch_size}.res
  done
done

