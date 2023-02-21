#include <stdio.h>
#include <stdint.h>
#include "wasm_export.h"

#include <map>
#include <set>
#include <mutex>

std::mutex mtx;
struct acc_entry {
  wasm_exec_env_t last_tid;
  uint64_t freq;
  bool shared;
  std::set<uint32_t> shared_inst_idxs;
  acc_entry() : freq(0), shared(0), last_tid(NULL) { };
};

/* Log data */
std::map<uint32_t, acc_entry> access_table;

void logaccess_wrapper(wasm_exec_env_t exec_env, uint32_t addr, uint32_t opcode, uint32_t inst_idx) {
  mtx.lock();
  acc_entry &entry = access_table[addr];
  bool new_tid_acc = exec_env != entry.last_tid;
  /* Flag as shared memory if different tids access it */
  if (entry.last_tid && new_tid_acc) {
    entry.shared = true;
  }
  /* Log indexes to conflicting address */
  if (entry.shared || new_tid_acc) {
    entry.shared_inst_idxs.insert(inst_idx);
  }
  entry.last_tid = exec_env;
  entry.freq += 1;
  mtx.unlock();
}

void logend_wrapper(wasm_exec_env_t exec_env) {
  //printf("=== ACCESS TABLE ===\n");
  //for (auto const& x : access_table) {
  //  const acc_entry &entry = x.second;
  //  const std::set<uint32_t> &shared_idxs = entry.shared_inst_idxs;
  //  if (x.second.shared) {
  //    printf("Addr [%u] | Accesses: %lu  [SHARED by %lu insts: ", 
  //      x.first, x.second.freq, shared_idxs.size());
  //    for (auto &v : shared_idxs) {
  //      printf("%u, ", v);
  //    }
  //    printf("]\n");
  //  } else {
  //    printf("Addr [%u] | Accesses: %lu (ISOLATED: %u)\n", 
  //      x.first, x.second.freq, *(shared_idxs.begin()));
  //  }
  //}
  //printf("================\n");

  //printf("=== Instruction Indexes ===\n");
  //printf("Num instructions: %u\n", shared_inst_idxs.size());
  //for (auto &idx : shared_inst_idxs) {
  //  printf("%d\n", idx);
  //}
  //printf("================\n");
}

#define REG_NATIVE_FUNC(func_name, sig) \
  { #func_name, (void*) func_name##_wrapper, sig, NULL }
static NativeSymbol native_symbols[] = {
  REG_NATIVE_FUNC(logaccess, "(iii)"),
  REG_NATIVE_FUNC(logend, "()")
};

extern "C" uint32_t get_native_lib (char **p_module_name, NativeSymbol **p_native_symbols) {
  *p_module_name = "instrument";
  *p_native_symbols = native_symbols;
  return sizeof(native_symbols) / sizeof(NativeSymbol);
}

