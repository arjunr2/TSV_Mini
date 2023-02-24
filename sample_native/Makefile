.PHONY: clean

native_fns.o: native_fns.cpp
	g++ -g -c -fPIC $< -o $@
	g++ -g -shared -o libnative.so $@ libvmlib.a
	
clean:
	rm -f *.o libnative.so
