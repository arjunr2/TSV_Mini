#!/bin/bash

VIOLATION_DIR=violation_logs

filename=$(basename ${1%.aot.tsvinst})
mkdir -p $VIOLATION_DIR
iwasm --native-lib=./libtsvd.so $1 | tee $VIOLATION_DIR/$filename.violation
