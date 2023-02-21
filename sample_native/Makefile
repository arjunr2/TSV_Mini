.PHONY: clean

native_fns.o: native_fns.cpp
	g++ -g -c -fPIC $< -o $@
	g++ -g -shared -o libnative.so $@
	
clean:
	rm -f *.o *.so
