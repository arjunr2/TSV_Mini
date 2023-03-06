#!/bin/bash

SHARED_ACC_DIR=shared_access

iwasm --native-lib=./libaccess.so $1
if [ $? -eq 0 ]; then
  filename=$(basename ${1%.aot.accinst})
  mkdir -p $SHARED_ACC_DIR
  mv shared_mem.bin $SHARED_ACC_DIR/$filename.shared_acc.bin
fi
