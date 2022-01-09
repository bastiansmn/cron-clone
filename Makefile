.PHONY: all

all : distclean cassini saturnd

client : distclean cassini

server : distclean saturnd

cassini : 
	gcc -Wall -Iinclude src/client/*.c src/*.c -o cassini

saturnd :
	gcc -Wall -Iinclude src/server/*.c src/*.c -o saturnd

test : client
	bash run-cassini-tests.sh

distclean:
	rm -rf src/*.o cassini saturnd
	@killall saturnd 2> /dev/null || true