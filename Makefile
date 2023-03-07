.PHONY: all clean clean-tests clean-tools

WASI_CLANG := /opt/wasi-sdk-16/bin/clang

WAMR_ROOT := /home/arjun/Documents/research/arena/wamr-conix
WAMRC := $(WAMR_ROOT)/wamr-compiler/build/wamrc

TEST_DIR := tests
WASM_DIR := wasms
AOT_DIR := aots
SHARED_ACC_DIR := shared_access
VIOLATION_DIR := violation_logs

TEST_C := $(notdir $(wildcard $(TEST_DIR)/*.c))

WASM_O := $(addprefix $(WASM_DIR)/, $(TEST_C:.c=.wasm))

#AARCH64_AOT_O := $(TEST_C:.c=.aarch64.aot)
X86_64_AOT_O := $(TEST_C:.c=.aot)
AOT_O := $(addprefix $(AOT_DIR)/, $(X86_64_AOT_O) $(AARCH64_AOT_O))


IMPORT_DIR := imports
INSTRUMENT_DIR := wasm-instrument

all: native-lib instrument tests


.PHONY: instrument
instrument:
	make -j6 -C $(INSTRUMENT_DIR)
	cp $(INSTRUMENT_DIR)/instrument .

.PHONY: native-lib
.ONESHELL:
native-lib:
	make -C $(IMPORT_DIR)
	cp $(IMPORT_DIR)/libaccess.so .
	cp $(IMPORT_DIR)/libtsvd.so .

.PHONY: tests setup
tests: setup $(AOT_O)
	mkdir -p $(SHARED_ACC_DIR)

.ONESHELL:
setup:
	mkdir -p $(WASM_DIR)
	mkdir -p $(AOT_DIR)
	make -C $(TEST_DIR)


# AOT Original + Access Compilation 
.ONESHELL:
$(AOT_DIR)/%.aot: $(WASM_DIR)/%.wasm
	$(WAMRC) --enable-multi-thread -o $@ $<
	$(WAMRC) --enable-multi-thread -o $@.accinst $<.accinst

.ONESHELL:
$(AOT_DIR)/%.aarch64.aot: $(WASM_DIR)/%.wasm
	$(WAMRC) --target=aarch64 --enable-multi-thread -o $@ $<
	$(WAMRC) --target=aarch64 --enable-multi-thread -o $@.accinst $<.accinst


# WASM Compilation + Access Instrumentation
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
	./instrument -s memaccess -o $@.accinst $@
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/$*.wat
	wasm2wat --enable-threads $@.accinst -o $(WASM_DIR)/$*.wat.accinst


# WASM TSV shared reinstrumentation
FILTER := shared_mem.bin
AOT_O_SHINST := $(addsuffix .tsvinst, $(AOT_O))
SHARED_ACC_BINS := $(notdir $(wildcard $(SHARED_ACC_DIR)/*.shared_acc.bin))
AOT_O_SHINST := $(addprefix $(AOT_DIR)/, $(SHARED_ACC_BINS:.shared_acc.bin=.aot.tsvinst))

shared-instrument: $(AOT_O_SHINST)

$(AOT_DIR)/%.aarch64.aot.tsvinst: $(WASM_DIR)/%.wasm.tsvinst
	$(WAMRC) --target=aarch64 --enable-multi-thread -o $@ $<

$(AOT_DIR)/%.aot.tsvinst: $(WASM_DIR)/%.wasm.tsvinst
	$(WAMRC) --enable-multi-thread -o $@ $<

$(WASM_DIR)/%.wasm.tsvinst: $(SHARED_ACC_DIR)/%.shared_acc.bin
	./instrument -s memshared -a $< -o $@ $(WASM_DIR)/$*.wasm
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/$*.wat.tsvinst


# Cleaning
clean-tests:
	#rm -f *.wasm *.wat *.aot *.so *.accinst *.tsvinst
	rm -rf $(WASM_DIR) $(AOT_DIR)
	make -C $(TEST_DIR) clean

clean-tools:
	make -C $(IMPORT_DIR) clean
	make -C $(INSTRUMENT_DIR) clean
	make -C $(TEST_DIR) clean

clean-logs:
	rm -rf $(SHARED_ACC_DIR) $(VIOLATION_DIR)

clean: clean-tools clean-tests
