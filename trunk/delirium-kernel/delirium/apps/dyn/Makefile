#GCC_EXEC_PREFIX=i686-shikita-delirium-
GCC_EXEC_PREFIX=
CC=$(GCC_EXEC_PREFIX)gcc
LD=$(GCC_EXEC_PREFIX)ld
AS=$(GCC_EXEC_PREFIX)as

#CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc -fpie -pie
CFLAGS=-Wall -g -DARCH_i386 -DENABLE_ASSERTS -ffreestanding -nostdinc 
CFLAGS+=-isystem../../include/ -I.. -I../lib -mpush-args
ASFLAGS= $(CFLAGS)
LDFLAGS=-static -L ../lib -pie

APPOBJS=calls-user.o crt0.o
LIBS=


default: crt0.o calls-user.o dyn

clean:
	rm *.o dyn 2>/dev/null || true

dyn: dyn.o
	$(LD) $(LDFLAGS) $(APPOBJS) $^ $(LIBS) -o $@
	#$(GCC_EXEC_PREFIX)strip dyn
