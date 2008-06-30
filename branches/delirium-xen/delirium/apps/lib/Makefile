#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
CFLAGS=-Wall -isystem../../include/ -I.. -fno-builtin -O2 -mpush-args

default: libdvga.a

libdvga.a: vga/vga.o vga/vga_modes.o
	$(AR) -rcs $@ $^

clean:
	rm *.a vga/*.o || true 2> /dev/null

fishy: ../crt0.o ../calls-user.o ../delibrium/delibrium.o readline.o dsh.o fishy.o testserial.o life.o \
	vga.o vga_modes.o
	$(LD) $(LDFLAGS) $^  -o $@
