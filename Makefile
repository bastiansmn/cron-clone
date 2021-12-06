.PHONY: all

all : distclean cassini test

cassini : 
	gcc -Wall -Iinclude src/*.c -o cassini

test :
	bash run-cassini-tests.sh

distclean:
	rm -rf src/*.o cassini