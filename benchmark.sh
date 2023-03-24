#!/bin/bash


case $1 in
  "access")
    RES_DIR=access_results
    mkdir -p $RES_DIR
    make
    make access

    for run_no in {1..20}; do
      for batch_size in {2..15}; do
        make -B access-batch BATCH_SIZE=$batch_size

        sleep 4

        identifier=${run_no}_${batch_size}
        # Run no instrumentation 
        ./run_tests.py --mode=normal all &> $RES_DIR/noinst_run_$identifier.res
        # Run all instructions instrumented access
        ./run_tests.py --mode=access all &> $RES_DIR/allinst_run_$identifier.res
        # Run sharded instrumentation access
        ./run_tests.py --mode=access --batch=$batch_size all &> $RES_DIR/run_$identifier.res
      done
    done
    ;;

  "tsv")
    RES_DIR=violation_results
    mkdir -p $RES_DIR
    make
    make tsv

    for run_no in {1..2}; do
      for batch_size in {2..3}; do
        for id in {10..20..10}; do
          make -B tsv-batch STOCH=$id BATCH_SIZE=$batch_size

          sleep 2
          
          identifier=${run_no}_${id}_${batch_size}
          # Run no instrumentation 
          ./run_tests.py --mode=normal $2 &> $RES_DIR/noinst_run_$identifier.res
          # Run all instructions filtered accesses instrumented
          ./run_tests.py --mode=tsv $2 &> $RES_DIR/allinst_run_$identifier.res
          # Run sharded tsvd with stochastic subsets of filtered accesses
          ./run_tests.py --mode=tsv --batch=$batch_size $2 &> $RES_DIR/run_$identifier.res
        done
      done
    done
    ;;

  *) echo "Invalid argument (access or tsv)" ;;
esac
