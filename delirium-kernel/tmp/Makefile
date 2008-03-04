#CC=gcc
#CFLAGS=-fast -parallel
CC=i686-shikita-delirium-gcc
CHOST="i686-pc-linux-gnu"
CFLAGS=-Wall -march=prescott -O2 -pipe -fomit-frame-pointer -mmmx -msse -msse2 -msse3 -m3dnow -mfpmath=sse
CFLAGS+=-DDELIRIUM -g

EVERYTHING=test
OBJS-test=test.o timing.o prime.o
OBJS-testm=testm.o timing.o prime.o

default: test

#--[ Helper defs etc ]-------- -- -  -

all: $(EVERYTHING)

clean-%:
	-rm $(OBJS-$(*)) $(*)

help:
	@echo -e "Targets:\t$(EVERYTHING)\n\t\tall, clean-[<target>]"

define compile-bin
	$(CC) $(CFLAGS) -o $@ $^ 
endef

#--[ Modules ]-------- -- -  -

default:

clean: $(foreach target,$(EVERYTHING), clean-$(target))

test: $(OBJS-test)
	$(compile-bin) -lc -ldelirium

testm: $(OBJS-testm)
	$(compile-bin) -lm -lc -ldelirium

a: a.o
	$(compile-bin) -lc -ldelirium
