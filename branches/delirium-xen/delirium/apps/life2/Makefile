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

default: life2

clean:
	rm *.o life2 2>/dev/null || true

life2: life2.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
	$(GCC_EXEC_PREFIX)strip life2
