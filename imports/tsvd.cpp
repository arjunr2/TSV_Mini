#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>

#include "wasm_export.h"
#include "wasmdefs.h"

#include <unordered_set>
#include <mutex>
#include <queue>
#include <atomic>

#define INSTRUMENT 1
#define TRACE_ACCESS 0

#define DELAY 500

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
struct access_record {
  wasm_exec_env_t tid;
  uint32_t inst_idx;
  access_type type;
  uint32_t addr;

  bool operator==(const access_record& record) const {
    return (tid == record.tid) && (inst_idx == record.inst_idx) 
          && (type == record.type) && (addr == record.addr);
  }
};

struct AccessRecordPairHashFunction {
  size_t operator()(const std::pair<access_record, access_record> &p) const {
    return std::hash<uint32_t>{}(p.first.inst_idx) ^ std::hash<uint32_t>{}(p.first.inst_idx);
  }
};
std::mutex violation_mtx;
std::unordered_set<std::pair<access_record, access_record>, AccessRecordPairHashFunction> violation_set;

struct tsv_entry {
  std::atomic_bool probe;
  std::atomic_llong freq_diff_tid_consec;
  access_record access;
  std::mutex access_mtx;
};
tsv_entry *tsv_table = NULL;
size_t table_size = sizeof(tsv_entry) * ((size_t)1 << 32);

struct inst_entry {
  std::atomic_llong freq;
};
inst_entry *instruction_map;
uint32_t inst_count;
/*  */


/* Delay without nanosleep since we don't want syscall overhead */
/* Delay is relative to processor speed */
inline void delay(uint32_t punits) {
  for (int i = 0; i < punits; i++) {
    asm volatile ("nop");
  }
}

void logaccess_wrapper(wasm_exec_env_t exec_env, uint32_t addr, uint32_t opcode, uint32_t inst_idx) {
  #if INSTRUMENT == 1
  access_record cur_access {exec_env, inst_idx, opcode_access[opcode].type, addr};
  #if TRACE_ACCESS == 1
  if (cur_access.type == STORE) 
    printf("I: %u | A: %u | T: %p (W)\n", cur_access.inst_idx, cur_access.addr, cur_access.tid); 
  else 
    printf("I: %u | A: %u | T: %p (R)\n", cur_access.inst_idx, cur_access.addr, cur_access.tid); 
  #endif

  tsv_entry *entry = tsv_table + addr;
  bool probed = entry->probe.exchange(true);
  /* If not probed, setup probe info and delay */
  if (!probed) {
    /* Access record must be updated atomically */
    entry->access_mtx.lock();
    entry->access = cur_access;
    entry->access_mtx.unlock();
    delay(DELAY);
    entry->probe.store(false);
  }
  /* If probed, check if at least one write from different thread */
  else {
    /* Access checked atomically */
    entry->access_mtx.lock();
    if (exec_env != entry->access.tid) {
      //printf("Conflict with %u\n", entry->access.inst_idx);
      if ((entry->access.type == STORE) || (cur_access.type == STORE)) {
        printf("Violation!\n");
        /* Log as violation */
        violation_mtx.lock();
        violation_set.insert(std::make_pair(entry->access, cur_access));
        violation_mtx.unlock();
      }
      entry->freq_diff_tid_consec++;
    }
    entry->access_mtx.unlock();
  }
  instruction_map[inst_idx].freq++;
  #endif
}


void logend_wrapper(wasm_exec_env_t exec_env) {
  end_ts = gettime();
  float total_time = (float)(end_ts - start_ts) / 1000000; 
  printf("========= LOGEND ===========\n");
  printf("Time taken: %.3f\n", total_time);

  #if INSTRUMENT == 1
  printf("Violations:\n");
  int i = 0;
  for (auto &violation : violation_set) {
    printf("Addr [%u|%u] by instructions (%u, %u) --> TID(%p, %p)\n", violation.first.addr, violation.second.addr, 
      violation.first.inst_idx, violation.second.inst_idx, violation.first.tid, violation.second.tid);
  }
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
  static std::atomic_bool init_done {false};
  /* Init functions should only happen once */
  if (first.exchange(true) == false) {
    init_tsv_table();
    printf("Num instructions: %u\n", num_instructions);
    inst_count = num_instructions;
    instruction_map = new inst_entry[num_instructions];
    init_done = true;
    start_ts = gettime();
  } 
  else {
    while (!init_done) { };
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

