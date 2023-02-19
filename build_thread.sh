#!/bin/bash

shopt -s expand_aliases
source ~/.alias

WAMR_ROOT=/home/arjun/Documents/research/arena/wamr-conix

name=$(basename $1 .c)

/opt/wasi-sdk-16/bin/clang --target=wasm32  \
    --sysroot=${WAMR_ROOT}/wamr-sdk/app/libc-builtin-sysroot   \
    -O3 -pthread -nostdlib -z stack-size=32768      \
    -Wl,--shared-memory             \
    -Wl,--allow-undefined-file=${WAMR_ROOT}/wamr-sdk/app/libc-builtin-sysroot/share/defined-symbols.txt \
    -Wl,--no-entry -Wl,--export=main                \
    -Wl,--export=__heap_base,--export=__data_end    \
    -Wl,--export=__wasm_call_ctors  \
    $name.c -o $name.wasm
# -pthread: it will enable some dependent WebAssembly features for thread
# -nostdlib: disable the WASI standard library as we are using WAMR builtin-libc
# -z stack-size=: specify the total aux stack size
# -Wl,--export=__heap_base,--export=__data_end: export these globals so the runtime can resolve the total aux stack size and the start offset of the stack top
# -Wl,--export=__wasm_call_ctors: export the init function to initialize the passive data segments

wasm2wat --enable-threads $name.wasm -o $name.wat
