#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc
CFLAGS+=-isystem../../include/ -I.. -I../lib -mpush-args
LDFLAGS=-static -L ../lib -Ttext=0x62020000

APPOBJS=../crt0.o ../calls-user.o
LIBS=-L../delibrium -ldelibrium

default: rtl8139

clean:
	rm *.o rtl8139 2>/dev/null || true

rtl8139: rtl8139.o pci.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
	#$(GCC_EXEC_PREFIX)strip rtl8139
