WASI_CLANG := /opt/wasi-sdk-16/bin/clang

WAMR_ROOT := /home/arjun/Documents/research/arena/wamr-conix
WAMRC := $(WAMR_ROOT)/wamr-compiler/build/wamrc

TEST_DIR := tests
WASM_DIR := wasms
AOT_DIR := aots
SHARED_ACC_DIR := shared_access
VIOLATION_DIR := violation_logs
IMPORT_DIR := imports
INSTRUMENT_DIR := wasm-instrument

# Base AOT/WASM Files
TEST_C := $(notdir $(wildcard $(TEST_DIR)/*.c))
TEST_WASM := $(TEST_C:.c=.wasm)

WASM_O := $(addprefix $(WASM_DIR)/, $(TEST_WASM))

X86_64_AOT_O := $(TEST_C:.c=.aot)

TEST_AOT := $(X86_64_AOT_O) $(AARCH64_AOT_O)
AOT_O := $(addprefix $(AOT_DIR)/, $(TEST_AOT))

# Access instrumented files
WASM_ACC_O := $(addsuffix .accinst, $(WASM_O))
AOT_ACC_O := $(addsuffix .accinst, $(AOT_O))

# TSV instrumented files
SHARED_ACC_BINS := $(notdir $(wildcard $(SHARED_ACC_DIR)/*.shared_acc.bin))
WASM_TSV_O := $(addprefix $(WASM_DIR)/, $(SHARED_ACC_BINS:.shared_acc.bin=.wasm.tsvinst))
AOT_TSV_O := $(addprefix $(AOT_DIR)/, $(SHARED_ACC_BINS:.shared_acc.bin=.aot.tsvinst))


# Batching variables
BATCH_SIZE := 2
BATCH_RANGE := $(shell seq --separator=' ' $(BATCH_SIZE)) 

TEST_WASM_ACC_BATCH := $(foreach wasm, $(TEST_WASM), $(foreach idx, $(BATCH_RANGE), p$(idx).$(wasm).accinst))
TEST_AOT_ACC_BATCH := $(TEST_WASM_ACC_BATCH:.wasm.accinst=.aot.accinst)

WASM_ACC_BATCH_O := $(addprefix $(WASM_DIR)/, $(TEST_WASM_ACC_BATCH))
AOT_ACC_BATCH_O := $(addprefix $(AOT_DIR)/, $(TEST_AOT_ACC_BATCH))

# Stochasticity param
STOCH := 60



.PHONY: base access
.PHONY: instrument native-lib 
.PHONY: clean clean-tests clean-tools
.PHONY: tests tests-batch setup

## BASE PHASE
base: native-lib instrument tests
instrument:
	make -j6 -C $(INSTRUMENT_DIR)
	cp $(INSTRUMENT_DIR)/instrument .

.ONESHELL:
native-lib:
	make -C $(IMPORT_DIR)
	cp $(IMPORT_DIR)/libaccess.so .
	cp $(IMPORT_DIR)/libtsvd.so .

tests: setup $(AOT_O)
.ONESHELL:
setup:
	mkdir -p $(WASM_DIR)
	mkdir -p $(AOT_DIR)
	make -C $(TEST_DIR)


## PHASE 1 (can happen along with base)
access: base $(AOT_ACC_O)
	mkdir -p $(SHARED_ACC_DIR)

access-batch: base $(AOT_ACC_BATCH_O)
	mkdir -p $(SHARED_ACC_DIR)

# Batch override for access inst: only run once
.SECONDARY: $(WASM_ACC_BATCH_O)
.ONESHELL:
$(WASM_DIR)/p1.%.wasm.accinst: $(WASM_DIR)/%.wasm
	./instrument -s memaccess-stochastic -a "$(STOCH) $(BATCH_SIZE)" -o $<.accinst $<
	for idx in $(BATCH_RANGE) ; do \
		wasm2wat --enable-threads $(WASM_DIR)/p$$idx.$*.wasm.accinst -o \
		$(WASM_DIR)/p$$idx.$*.wat.accinst;	\
	done


## PHASE 2
tsv: base $(AOT_TSV_O)
	mkdir -p $(VIOLATION_DIR)

# Input batchfile (can be a normal single file, batch file, or p file)
.ONESHELL:
$(WASM_DIR)/batch.%.wasm.tsvinst: $(SHARED_ACC_DIR)/batch.%.shared_acc.bin $(WASM_DIR)/%.wasm
	./instrument -s memshared -a $< -o $@ $(WASM_DIR)/$*.wasm
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/batch.$*.wat.tsvinst

$(WASM_DIR)/p%.wasm.tsvinst: $(SHARED_ACC_DIR)/batch.%.shared_acc.bin $(WASM_DIR)/%.wasm
	./instrument -s memshared -a $< -o $@ $(WASM_DIR)/$*.wasm
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/batch.$*.wat.tsvinst




## BASE PHASE
# WASM and AoT Compilation
$(AOT_DIR)/%.aot: $(WASM_DIR)/%.wasm
	$(WAMRC) --enable-multi-thread -o $@ $<

.SECONDARY: $(WASM_O)
.ONESHELL:
$(WASM_DIR)/%.wasm: $(TEST_DIR)/%.c
	$(WASI_CLANG) -g --target=wasm32  \
			--sysroot=$(WAMR_ROOT)/wamr-sdk/app/libc-builtin-sysroot   \
			-O3 -pthread -nostdlib -z stack-size=32768      \
			-Wl,--shared-memory             \
			-Wl,--allow-undefined-file=$(WAMR_ROOT)/wamr-sdk/app/libc-builtin-sysroot/share/defined-symbols.txt \
			-Wl,--no-entry -Wl,--export=main                \
			-Wl,--export=__heap_base,--export=__data_end    \
			-Wl,--export=__wasm_call_ctors  \
			$< $(TEST_DIR)/liblfds711.a -o $@
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/$*.wat


## PHASE 1
# WASM Access Instrumentation
$(AOT_DIR)/%.aot.accinst: $(WASM_DIR)/%.wasm.accinst
	$(WAMRC) --enable-multi-thread -o $@ $<

.SECONDARY: $(WASM_ACC_O)
.ONESHELL:
$(WASM_DIR)/%.wasm.accinst: $(WASM_DIR)/%.wasm
	./instrument -s memaccess -o $@ $(WASM_DIR)/$*.wasm
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/$*.wat.accinst


## PHASE 2
# WASM TSV Shared Reinstrumentation
$(AOT_DIR)/%.aot.tsvinst: $(WASM_DIR)/%.wasm.tsvinst
	$(WAMRC) --enable-multi-thread -o $@ $<

.SECONDARY: $(WASM_TSV_O)
$(WASM_DIR)/%.wasm.tsvinst: $(SHARED_ACC_DIR)/%.shared_acc.bin $(WASM_DIR)/%.wasm 
	./instrument -s memshared -a $< -o $@ $(WASM_DIR)/$*.wasm
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/$*.wat.tsvinst


# Cleaning
clean: clean-tools clean-tests

clean-tests:
	rm -rf $(WASM_DIR) $(AOT_DIR)
	make -C $(TEST_DIR) clean

clean-tools:
	make -C $(IMPORT_DIR) clean
	make -C $(INSTRUMENT_DIR) clean
	make -C $(TEST_DIR) clean

clean-logs:
	rm -rf $(SHARED_ACC_DIR) $(VIOLATION_DIR)

