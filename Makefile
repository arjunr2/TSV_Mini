.PHONY: all clean clean-tests clean-tools

WASI_CLANG := /opt/wasi-sdk-16/bin/clang

WAMR_ROOT := /home/arjun/Documents/research/arena/wamr-conix
WAMRC := $(WAMR_ROOT)/wamr-compiler/build/wamrc

TEST_DIR := tests
WASM_DIR := wasms
AOT_DIR := aots

TEST_C := $(notdir $(wildcard $(TEST_DIR)/*.c))
WASM_O := $(addprefix $(WASM_DIR)/, $(TEST_C:.c=.wasm))
AOT_O := $(addprefix $(AOT_DIR)/, $(TEST_C:.c=.aot))


NATIVE_DIR := sample_native
INSTRUMENT_DIR := wasm-instrument

all: native-lib tests

tdirs:
	mkdir -p $(WASM_DIR)
	mkdir -p $(AOT_DIR)

instrument:
	make -j4 -C $(INSTRUMENT_DIR)
	cp $(INSTRUMENT_DIR)/instrument .

native-lib:
	make -C $(NATIVE_DIR)
	cp $(NATIVE_DIR)/libnative.so .


tests: tdirs $(AOT_O)

$(AOT_DIR)/%.aot: $(WASM_DIR)/%.wasm
	$(WAMRC) --enable-multi-thread -o $@ $<
	$(WAMRC) --enable-multi-thread -o $@.inst $<.inst

.SECONDARY: $(WASM_O)
$(WASM_DIR)/%.wasm: $(TEST_DIR)/%.c instrument
	$(WASI_CLANG) --target=wasm32  \
			--sysroot=$(WAMR_ROOT)/wamr-sdk/app/libc-builtin-sysroot   \
			-O3 -pthread -nostdlib -z stack-size=32768      \
			-Wl,--shared-memory             \
			-Wl,--allow-undefined-file=$(WAMR_ROOT)/wamr-sdk/app/libc-builtin-sysroot/share/defined-symbols.txt \
			-Wl,--no-entry -Wl,--export=main                \
			-Wl,--export=__heap_base,--export=__data_end    \
			-Wl,--export=__wasm_call_ctors  \
			$< -o $@
	./instrument -o $@.inst $@
	wasm2wat --enable-threads $@ -o $(WASM_DIR)/$*.wat


clean-tests:
	rm -f *.wasm *.wat *.aot *.so instrument
	rm -rf wasms aots

clean-tools:
	make -C $(NATIVE_DIR) clean
	make -C $(INSTRUMENT_DIR) clean

clean: clean-tools clean-tests
