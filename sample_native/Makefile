.PHONY: all clean

all:
	g++ -c -fPIC native_fns.cpp -o native_fns.o
	g++ -shared -o libnative.so native_fns.o
	
clean:
	rm *.o *.so
