CC      ?= gcc
CFLAGS  ?= -Wall

.PHONY: all

all : cassini

cassini : 
	$(CC) $(CFLAGS) -Iinclude src/cassini.c -o cassini

test :
	bash run-cassini-tests.sh

distclean:
	rm -r src/*.o cassini