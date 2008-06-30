#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
CFLAGS=-Wall -g -isystem../../include/ -I.. -fno-builtin -O2 -mpush-args

default: fishy

clean:
	rm *.o fishy || true 2> /dev/null

fishy: ../crt0.o ../calls-user.o ../delibrium/delibrium.o readline.o dsh.o fishy.o 
	$(LD) $(LDFLAGS) $^  -o $@
	$(GCC_EXEC_PREFIX)strip fishy
