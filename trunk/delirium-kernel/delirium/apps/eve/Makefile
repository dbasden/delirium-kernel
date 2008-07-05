# See Makefile.delirium
#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
LDFLAGS+=-static -Ttext 0x60000000
CFLAGS=-Wall -g -isystem../../include/ -I.. -fno-builtin -O2 -mpush-args

default: eve

clean:
	rm *.o eve 2>/dev/null || true

eve: ../crt0.o ../calls-user.o ../delibrium/delibrium.o readline.o dsh.o elf.o eve.o
	$(LD) $(LDFLAGS) $^  -o $@
	#$(GCC_EXEC_PREFIX)strip eve
