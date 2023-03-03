#pragma once
#include "../wasm-instrument/wasmops.h"
#include <stdint.h>

/* Must be a C-compatible header for designated init */
typedef enum {
  NOACCESS = 0,
  STORE,
  LOAD,
} access_type;

typedef struct {
  access_type type;
  uint8_t width;
} opaccess;

/* Defined in C file so we can use designated initializers */
extern opaccess opcode_access[];

