#include <stdio.h>
#include <stdint.h>
#include "wasm_export.h"

#include <map>

struct acc_entry {
  wasm_exec_env_t last_tid;
  uint64_t freq;
  bool shared;
  acc_entry() : freq(0), shared(0), last_tid(NULL) { };
};

std::map<uint32_t, acc_entry> access_table;

void logaccess_wrapper(wasm_exec_env_t exec_env, uint32_t addr, uint32_t opcode) {
  acc_entry &entry = access_table[addr];
  if (entry.last_tid && (exec_env != entry.last_tid)) {
    entry.shared = true;
  }
  entry.last_tid = exec_env;
  entry.freq += 1;
}

void logend_wrapper(wasm_exec_env_t exec_env) {
  printf("=== ACCESS TABLE ===\n");
  for (auto const& x : access_table) {
    if (x.second.shared) {
      printf("Addr [%u] | Accesses: %lu [SHARED]\n", x.first, x.second.freq);
    } else {
      printf("Addr [%u] | Accesses: %lu\n", x.first, x.second.freq);
    }
  }
  printf("================\n");
}

#define REG_NATIVE_FUNC(func_name, sig) \
  { #func_name, (void*) func_name##_wrapper, sig, NULL }
static NativeSymbol native_symbols[] = {
  REG_NATIVE_FUNC(logaccess, "(ii)"),
  REG_NATIVE_FUNC(logend, "()")
};

extern "C" uint32_t get_native_lib (char **p_module_name, NativeSymbol **p_native_symbols) {
  *p_module_name = "instrument";
  *p_native_symbols = native_symbols;
  return sizeof(native_symbols) / sizeof(NativeSymbol);
}

