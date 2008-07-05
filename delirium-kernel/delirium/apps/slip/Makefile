#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc
CFLAGS+=-isystem../../include/ -I.. -I../lib -mpush-args
LDFLAGS=-static -L ../lib -Ttext 0x65020000

APPOBJS=../delibrium/delibrium.o ../crt0.o -R ../calls-user.o
LIBS=

default: slip

clean:
	rm *.o slip 2>/dev/null || true

slip: slip.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
	#$(GCC_EXEC_PREFIX)strip slip
