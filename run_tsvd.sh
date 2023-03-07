#!/bin/bash

# "./run_tsvd all" to run all TSVD tests
# Otherwise "./run_tsvd <test>.aot.tsvinst" for single run

VIOLATION_DIR=violation_logs
AOT_DIR=aots

run_script() {
  echo "--> Test \"$1\" <--"
  mkdir -p $VIOLATION_DIR
  filename=$(basename ${1%.aot.tsvinst})
  iwasm --native-lib=./libtsvd.so $1 | tee $VIOLATION_DIR/$filename.violation
}


if [ "$1" = "all" ]; then
  aots=$AOT_DIR/*.aot.tsvinst
  echo $aots
  for aot in $aots; do
    run_script $aot
  done
else
  run_script $1
fi
