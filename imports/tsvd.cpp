#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "wasm_export.h"
#include "wasmops.h"

#include <map>
#include <mutex>
#include <vector>
#include <list>
#include <fstream>
#include <atomic>

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

/* TSV Access Logging */
struct tsv_entry {
  wasm_exec_env_t last_tid;
  uint32_t last_inst_idx;
  bool write;
  uint64_t freq_diff_tid_consec;
};

std::mutex mtx;
tsv_entry *tsv_table = NULL;
size_t table_size = sizeof(tsv_entry) * ((size_t)1 << 32);

uint32_t *instruction_map;

std::list<std::pair<uint32_t, uint32_t>> violation_list;

/*  */


void logaccess_wrapper(wasm_exec_env_t exec_env, uint32_t addr, uint32_t opcode, uint32_t inst_idx) {
  #if INSTRUMENT == 1
  mtx.lock();
  #if TRACE_ACCESS == 1
  printf("I: %u | A: %u | T: %p\n", inst_idx, addr, exec_env); 
  #endif
  tsv_entry *entry = tsv_table + addr;
  bool new_tid_acc = (exec_env != entry->last_tid);

  mtx.unlock();
  #endif
}


void logend_wrapper(wasm_exec_env_t exec_env) {
  end_ts = gettime();
  float total_time = (float)(end_ts - start_ts) / 1000000; 
  printf("========= LOGEND ===========\n");
  printf("Time taken: %.3f\n", total_time);

  #if INSTRUMENT == 1
  #endif
  int status = munmap(tsv_table, table_size);
  if (status == -1) {
    perror("munmap error");
  }
  delete instruction_map;
}



/* Initialization routine */
void init_tsv_table() {
  tsv_table = (tsv_entry*) mmap(NULL, table_size, PROT_READ|PROT_WRITE, 
                    MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE, -1, 0);
  if (tsv_table == NULL) {
    perror("malloc error");
  }
}

void logstart_wrapper(wasm_exec_env_t exec_env, uint32_t num_instructions) {
  static std::atomic_bool first {false};
  static std::atomic_bool first_done {false};
  if (first.exchange(true) == false) {
    init_tsv_table();
    printf("Num instructions: %u\n", num_instructions);
    instruction_map = new uint32_t[num_instructions];
    start_ts = gettime();
    first_done = true;
  } 
  else {
    while (!first_done) { };
  }
}



/* WAMR Registration Hooks */
#define REG_NATIVE_FUNC(func_name, sig) \
  { #func_name, (void*) func_name##_wrapper, sig, NULL }
static NativeSymbol native_symbols[] = {
  REG_NATIVE_FUNC(logstart, "(i)"),
  REG_NATIVE_FUNC(logaccess, "(iii)"),
  REG_NATIVE_FUNC(logend, "()")
};


extern "C" uint32_t get_native_lib (char **p_module_name, NativeSymbol **p_native_symbols) {
  *p_module_name = "instrument";
  init_tsv_table();
  *p_native_symbols = native_symbols;
  return sizeof(native_symbols) / sizeof(NativeSymbol);
}

