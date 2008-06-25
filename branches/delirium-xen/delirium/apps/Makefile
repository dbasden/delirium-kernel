# See Makefile.delirium
#`GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
AR=$(GCC_EXEC_PREFIX)ar
LD=$(GCC_EXEC_PREFIX)ld
CFLAGS=-Wall -ffreestanding -Wsystem-headers -nostdlib -g -DARCH_i386 -DENABLE_ASSERTS -nostdinc 
CFLAGS+=-isystem../include -static
ASFLAGS=$(CFLAGS)
LDFLAGS=$(CFLAGS)

APPOBJS=crt0.o calls-user.o
APPS=eve/eve test/test.elf fishy/fishy life/life testserial/testserial life2/life2
APPS+=vgatest/vgatest testfaults/testfaults

default: appobjs libs apps

appobjs: $(APPOBJS)

apps: $(APPS) libs

libs: delibrium/delibrium.o
	make -C lib

delibrium/delibrium.o:
	make -C delibrium

eve/eve:
	make -C eve

test/test.elf:
	make -C test

test/testfaults:
	make -C testfaults

life/life:
	make -C life

life2/life2:
	make -C life2

fishy/fishy:
	make -C fishy

testserial/testserial:
	make -C testserial

vgatest/vgatest:
	make -C vgatest

clean:
	make -C lib clean
	make -C delibrium clean
	make -C life clean
	make -C test clean
	make -C testfaults clean
	make -C fishy clean
	make -C testserial clean
	make -C vgatest clean
	make -C eve clean
	rm $(APPOBJS) 2>/dev/null || true
