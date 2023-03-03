.PHONY: clean

SRC_CPP := access.cpp tsvd.cpp
SRC_C := optable.c
SRC_O := $(SRC_CPP:.cpp=.o)

all: $(SRC_O)

optable.o: optable.c
	gcc -c $< -o $@ 

tsvd.o: tsvd.cpp optable.o
	g++ -std=c++2a -g -c -fPIC $< -o $@
	g++ -std=c++2a -g -shared -o lib$*.so optable.o $@ libvmlib.a

%.o: %.cpp
	g++ -g -c -fPIC $< -o $@
	g++ -g -shared -o lib$*.so $@ libvmlib.a

clean:
	rm -f *.o *.so
