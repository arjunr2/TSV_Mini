#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "wasm_export.h"

#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <fstream>

#define INSTRUMENT 1

/* Timing */
uint64_t start_ts;
uint64_t end_ts;

static inline uint64_t ts2us(struct timespec ts) {
  return (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);
}

uint64_t gettime() {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts2us(ts);
}
/* */


/* Access Logger */
struct acc_entry {
  wasm_exec_env_t last_tid;
  uint64_t freq;
  bool shared;
  acc_entry() : last_tid(NULL), freq(0), shared(0) { };
};

std::mutex mtx;
acc_entry *access_table = NULL;
uint32_t addr_min = -1;
uint32_t addr_max = 0;

std::set<uint32_t> shared_inst_idxs;
/*  */


void logaccess_wrapper(wasm_exec_env_t exec_env, uint32_t addr, uint32_t opcode, uint32_t inst_idx) {
  #if INSTRUMENT == 1
  mtx.lock();
  acc_entry *entry = access_table + addr;
  bool new_tid_acc = (exec_env != entry->last_tid);
  /* Flag as shared memory if different tids access it */
  if (entry->last_tid && new_tid_acc) {
    entry->shared = true;
  }
  /* Log indexes to conflicting address */
  if (entry->shared || new_tid_acc) {
    shared_inst_idxs.insert(inst_idx);
  }
  entry->last_tid = exec_env;
  entry->freq += 1;
  addr_min = (addr < addr_min) ? addr : addr_min;
  addr_max = (addr > addr_max) ? addr : addr_max;
  mtx.unlock();
  #endif
}


void logend_wrapper(wasm_exec_env_t exec_env) {
  end_ts = gettime();
  float total_time = (float)(end_ts - start_ts) / 1000000; 
  printf("========= LOGEND ===========\n");
  printf("Time taken: %.3f\n", total_time);

  #if INSTRUMENT == 1
  std::vector<uint32_t> shared_addrs;
  /* Read memory size from wamr API */
  uint32_t mem_size = wasm_runtime_get_memory_size(get_module_inst(exec_env));
  if (addr_max > mem_size) {
    printf("ERROR in mem size (%u) calculation (less than max addr: %u)\n", mem_size, addr_max);
  }
  printf("Mem size: %u\n", mem_size);
  printf("Addr min: %u | Addr max: %u\n", addr_min, addr_max);
  printf("=== ACCESS TABLE ===\n");
  for (uint64_t i = 0; i < mem_size; i++) {
    acc_entry *entry = access_table + i;
    if (entry->last_tid) {
      if (entry->shared) {
        printf("Addr [%lu] | Accesses: %lu [SHARED]\n", i, entry->freq);
        shared_addrs.push_back(i);
      } else {
        printf("Addr [%lu] | Accesses: %lu\n", i, entry->freq);
      }
    }
  }

  std::ofstream outfile("shared_mem.bin", std::ios::out | std::ios::binary);
  uint64_t num_bytes = shared_addrs.size() * sizeof(uint32_t);
  outfile.write((const char*) shared_addrs.data(), num_bytes);
  #endif
}



/* Initialization routine */
void init_acc_table() {
  size_t size = ((size_t)1 << 32);
  access_table = (acc_entry*) mmap(NULL, sizeof(acc_entry) * (((size_t)1) << 32), 
                                    PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  if (access_table == NULL) {
    perror("malloc error");
  }
  start_ts = gettime();
}


/* WAMR Registration Hooks */
#define REG_NATIVE_FUNC(func_name, sig) \
  { #func_name, (void*) func_name##_wrapper, sig, NULL }
static NativeSymbol native_symbols[] = {
  REG_NATIVE_FUNC(logaccess, "(iii)"),
  REG_NATIVE_FUNC(logend, "()")
};


extern "C" uint32_t get_native_lib (char **p_module_name, NativeSymbol **p_native_symbols) {
  *p_module_name = "instrument";
  init_acc_table();
  *p_native_symbols = native_symbols;
  return sizeof(native_symbols) / sizeof(NativeSymbol);
}

