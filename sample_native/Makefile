.PHONY: clean

native_fns.o: native_fns.cpp
	g++ -c -fPIC $< -o $@
	g++ -shared -o libnative.so $@
	
clean:
	rm -f *.o *.so
