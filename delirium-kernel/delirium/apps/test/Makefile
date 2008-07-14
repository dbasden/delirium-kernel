#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -mpush-args
CFLAGS+=-isystem../../include/ -I.. -I../lib -L ../lib -static

default: test.elf

clean:
	rm *.o test.elf 2>/dev/null || true

test.elf: test.o
	$(LD) $(LDFLAGS) $^ ../delibrium/delibrium.o ../crt0.o ../calls-user.o -o $@
	#$(GCC_EXEC_PREFIX)strip test.elf
