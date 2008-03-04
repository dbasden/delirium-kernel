CC=gcc
CFLAGS=-O2 -Wall  -DDEBUG


default: ramtreemaker

clean:
	rm *.o ramtreemaker fishy 2>/dev/null || true

test: ramtreemaker
	echo "Makefile image/imagefile ramtree.h image/src/h/ramtree ramtreemaker.c rtm.c" | ./ramtreemaker fishy
