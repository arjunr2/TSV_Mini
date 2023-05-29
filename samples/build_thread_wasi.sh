#!/bin/bash

shopt -s expand_aliases
source ~/.alias

WAMR_ROOT=/home/arjun/Documents/research/arena/wamr-conix
WAMRC=$WAMR_ROOT/wamr-compiler/build/wamrc

name=$(basename $1 .c)

/opt/wasi-sdk-20/bin/clang --target=wasm32-wasi-threads \
    -O3 -pthread       \
    -Wl,--shared-memory             \
    -Wl,--allow-undefined,--no-check-features \
    -Wl,--export=__heap_base,--export=__data_end    \
    $name.c -o $name.wasm
# -pthread: it will enable some dependent WebAssembly features for thread
# -nostdlib: disable the WASI standard library as we are using WAMR builtin-libc
# -z stack-size=: specify the total aux stack size
# -Wl,--export=__heap_base,--export=__data_end: export these globals so the runtime can resolve the total aux stack size and the start offset of the stack top
# -Wl,--export=__wasm_call_ctors: export the init function to initialize the passive data segments

wasm2wat --enable-threads $name.wasm -o $name.wat
$WAMRC --enable-multi-thread -o $name.aot $name.wasm
