#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc
CFLAGS+=-isystem../../include/ -I.. -I../lib
LDFLAGS=-static -L ../lib

APPOBJS=../delibrium/delibrium.o ../crt0.o ../calls-user.o

default: testfaults

clean:
	rm *.o testfaults 2>/dev/null || true

testfaults: testfaults.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ -o $@
	$(GCC_EXEC_PREFIX)strip testfaults
