.PHONY: clean

accesses.o: accesses.cpp
	g++ -g -c -fPIC $< -o $@
	g++ -g -shared -o libaccess.so $@ libvmlib.a
	
clean:
	rm -f *.o libaccess.so
