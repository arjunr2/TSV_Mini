.PHONY: all clean

all: instrument native-lib

instrument:
	make -j4 -C wasm-instrument
	cp wasm-instrument/instrument .

native-lib:
	make -C sample_native
	cp sample_native/libnative.so .

clean:
	rm -f *.wasm *.wat *.so instrument
