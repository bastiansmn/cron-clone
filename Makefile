CC      ?= gcc
CFLAGS  ?= -Wall

.PHONY: all

all : distclean cassini test

cassini : 
	$(CC) $(CFLAGS) -Iinclude src/*.c -o cassini

test :
	bash run-cassini-tests.sh

distclean:
	rm -rf src/*.o cassini