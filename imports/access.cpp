#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "wasm_export.h"

#include <map>
#include <set>
#include <unordered_set>
#include <mutex>
#include <vector>
#include <fstream>

#define INSTRUMENT 1
#define TRACE_ACCESS 0

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

typedef std::unordered_set<uint32_t> InstSet;

/* Access Logger */
struct acc_entry {
  wasm_exec_env_t last_tid;
  InstSet inst_idxs;
  uint64_t freq;
  bool shared;
};

std::mutex mtx;
acc_entry *access_table = NULL;
size_t table_size = sizeof(acc_entry) * ((size_t)1 << 32);
uint32_t addr_min = -1;
uint32_t addr_max = 0;

std::set<uint32_t> shared_inst_idxs;
/*  */


void logaccess_wrapper(wasm_exec_env_t exec_env, uint32_t addr, uint32_t opcode, uint32_t inst_idx) {
  #if INSTRUMENT == 1
  mtx.lock();
  #if TRACE_ACCESS == 1
  printf("I: %u | A: %u\n", inst_idx, addr); 
  #endif
  acc_entry *entry = access_table + addr;
  bool new_tid_acc = (exec_env != entry->last_tid);
  /* First access to address: Construct instruction set */
  if (!entry->last_tid) {
    new (&entry->inst_idxs) InstSet;
    entry->inst_idxs.insert(inst_idx);
  }
  /* Shared accesses from any thread write to global set */
  else if (entry->shared) {
    shared_inst_idxs.insert(inst_idx);
  }
  /* Unshared access from new thread: Mark as shared and append logged insts */
  else if (new_tid_acc) {
    entry->shared = true;
    shared_inst_idxs.insert(entry->inst_idxs.begin(), entry->inst_idxs.end());
    /* Save some memory by deleting unused set */
    entry->inst_idxs.~InstSet();
  }
  /* Unshared access from only one thread: Log inst */
  else {
    entry->inst_idxs.insert(inst_idx);
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
  char logfile[] = "shared_mem.bin";
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
      } else {
        printf("Addr [%lu] | Accesses: %lu\n", i, entry->freq);
      }
    }
  }

  std::ofstream outfile(logfile, std::ios::out | std::ios::binary);
  std::vector<uint32_t> inst_idxs(shared_inst_idxs.begin(), shared_inst_idxs.end());
  uint64_t num_bytes = inst_idxs.size() * sizeof(uint32_t);
  for (auto &i : inst_idxs) {
    printf("%u ", i);
  }
  printf("\n");
  outfile.write((char*) inst_idxs.data(), num_bytes);
  printf("Written data to %s\n", logfile);
  #endif
  int status = munmap(access_table, table_size);
  if (status == -1) {
    perror("munmap error");
  }
}



/* Initialization routine */
void init_acc_table() {
  access_table = (acc_entry*) mmap(NULL, table_size, PROT_READ|PROT_WRITE, 
                    MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
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

