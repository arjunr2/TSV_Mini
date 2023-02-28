.PHONY: clean

SRC := access.cpp tsvd.cpp
SRC_O := $(SRC:.cpp=.o)

all: $(SRC_O)

%.o: %.cpp
	g++ -g -c -fPIC $< -o $@
	g++ -g -shared -o lib$*.so $@ libvmlib.a

clean:
	rm -f *.o *.so
