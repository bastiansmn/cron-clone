CC      ?= gcc
CFLAGS  ?= -Wall


all : cassini

cassini : src/cassini.c
    $(CC) $(CFLAGS) -o cassini cassini.c $@

test :
	bash run-cassini-tests.sh

clean:
	rm -r src/*.o
