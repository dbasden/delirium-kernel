GCC_EXEC_PREFIX=i686-shikita-delirium-
CC=$(GCC_EXEC_PREFIX)gcc
CFLAGS=-Wall -ffreestanding -Wsystem-headers -mpush-args -nostdlib -g -DARCH_i386 -DENABLE_ASSERTS -nostdinc -isystem../delirium/include/ -I../delirium/include  -I ../apps 
ASFLAGS=$(CFLAGS)

default: ip.o slip.o

clean:
	rm *.o || true 2>/dev/null
