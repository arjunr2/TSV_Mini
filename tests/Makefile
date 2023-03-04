.PHONY: all clean

LIBS=lfds711

LFDS_SRC=liblfds711_src

all: $(LIBS)

.ONESHELL:
lfds711:
	make -C $(LFDS_SRC)/build/gcc_gnumake
	cp -r $(LFDS_SRC)/inc/* $(LFDS_SRC)/bin/liblfds711.a .

clean:
	rm -rf liblfds711.a
