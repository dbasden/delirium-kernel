#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc
CFLAGS+=-isystem../../include/ -I.. -I../lib
LDFLAGS=-static -L ../lib

APPOBJS=../delibrium/delibrium.o ../crt0.o ../calls-user.o
LIBS=-ldvga

default: life

clean:
	rm *.o life 2>/dev/null || true

life: life.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
	$(GCC_EXEC_PREFIX)strip life
