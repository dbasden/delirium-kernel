#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc
CFLAGS+=-isystem../../include/ -I.. -I../lib -mpush-args
LDFLAGS=-static -L ../lib -Ttext 0x65020000 --gc-sections --print-gc-sections

APPOBJS=../crt0.o -R ../calls-user.o
LIBS=-L../delibrium -ldelibrium
IPV4OBJS=ip_header.o ipv4.o tcp.o

default: slip

clean:
	rm *.o slip 2>/dev/null || true

slip: $(IPV4OBJS) slip.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
