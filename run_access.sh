#!/bin/bash

# "./run_access all" to run all access profiling tests
#     Does not run "batch" or "part" files
#
# "./run_access batch <test_basename>" to run batched set for a test
#     eg: for thread_lock test, do "./run_access batch thread_lock"
#     Runs all "part" files to generate a "batch" file for the test
# "./run_access batch all" to run batched set for all tests
#
# Otherwise "./run_access <test>.aot.accinst" for single run

SHARED_ACC_DIR=shared_access
AOT_DIR=aots
TEST_DIR=tests

run_script() {
  echo "--> Test \"$1\" <--"
  filename=$(basename ${1%.aot.accinst})
  iwasm --native-lib=./libaccess.so $1 > /dev/null
  if [ $? -eq 0 ]; then
    mkdir -p $SHARED_ACC_DIR
    mv shared_mem.bin $SHARED_ACC_DIR/$filename.shared_acc.bin
  else
    echo "Failed to generate $filename"
  fi
}


if [ "$1" = "all" ]; then
  tests=$(find $AOT_DIR/*.aot.accinst | grep -v -E '(/part|/batch)')
  echo $tests
  for testx in $tests; do
    run_script $testx
  done

elif [ "$1" = "batch" ]; then
  test_names=$2
  for test_name in $test_names; do
    subtests=$AOT_DIR/part*.$test_name.aot.accinst
    for subtest in $subtests; do
      run_script $subtest
    done
    cat $SHARED_ACC_DIR/part*.$test_name.shared_acc.bin | python3 dump_bin.py $SHARED_ACC_DIR/batch.$test_name.shared_acc.bin
    rm $SHARED_ACC_DIR/part*.$test_name.shared_acc.bin
  done

fi
