#!/bin/bash

# "./run_access all" to run all access profiling tests
# Otherwise "./run_access <test>.aot.accinst" for single run

SHARED_ACC_DIR=shared_access
AOT_DIR=aots

run_script() {
  echo "--> Test \"$1\" <--"
  iwasm --native-lib=./libaccess.so $1
  filename=$(basename ${1%.aot.accinst})
  if [ $? -eq 0 ]; then
    mkdir -p $SHARED_ACC_DIR
    mv shared_mem.bin $SHARED_ACC_DIR/$filename.shared_acc.bin
  else
    echo "Failed to generate $filename"
  fi
}

if [ "$1" = "all" ]; then
  aots=$AOT_DIR/*.aot.accinst
  for aot in $aots; do
    run_script $aot
  done
else
  run_script $1
fi
