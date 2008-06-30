#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc
CFLAGS+=-isystem../../include/ -I.. -I../lib -mpush-args
LDFLAGS=-static -L ../lib

APPOBJS=../delibrium/delibrium.o ../crt0.o ../calls-user.o
LIBS=-ldvga

default: vgatest

clean:
	rm *.o vgatest 2>/dev/null || true

vgatest: vgatest.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
	$(GCC_EXEC_PREFIX)strip vgatest
