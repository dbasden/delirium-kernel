include Makefile.delirium
include arch/${ARCH}/Makefile.arch

KERNLDFLAGS=-O1 -nostdlib -Map kern.map --cref -T./kernel.x
#KERNLDFLAGS=-nostdlib -Map kern.map --cref -Ttext ${KERNSTART} 

# TODO: Move into arch/fake/Makefile.arch
ifeq ("${ARCH}", "fake")
	KERNLDFLAGS=
	LD=${CC}
endif

# Objects used by the kernel in dlib
#
DLIBOBJS=../dlib/bsearchtree.o ../dlib/map.o ../dlib/memherd.o

KERNOBJS=kernel/kernel.o kernel/klib.o kernel/pool.o kernel/kvga.o
KERNOBJS+=kernel/ipc.o kernel/soapbox.o kernel/rant.o
KERNOBJS+=kernel/frameherd.o kernel/paging.o kernel/kthump.o kernel/elf.o
KERNOBJS+=kernel/ramtree.o

ifdef ENABLE_GDB_STUB
KERNOBJS+=kernel/gdb-support.o kernel/gdb-stub.o
endif
ifdef ENABLE_DEBUG_THREAD
KERNOBJS+=kernel/debug.o
endif


default: kern-${ARCH} apps symtab

symtab:
	nm ./kern-${ARCH} | awk '{print $$1 " " $$3}' > syms

apps: _apps

_apps:
	$(MAKE) -C apps

# Make sure we link ARCHOBJS first. The multiboot header must be in the
# first 8k of the ELF binary
#
# TODO: Write a linker script instead
#
kern-${ARCH}: ${ARCHOBJS} ${DLIBOBJS} ${KERNOBJS}
	$(LD) ${KERNLDFLAGS} -o $@ $^ 


ramtreemaker:
	make -C tools/ramtreemaker

ramtree: ramtreemaker
	tools/ramtreemaker/ramtreemaker ./delirium.ramtree < ramtree.src
	sudo mount -oloop ./floppy.386 ./mnt
	cp ./delirium.ramtree mnt
	sudo umount ./mnt
	/sbin/e2fsck floppy.386
	touch floppy.386
	
image: kern-${ARCH} apps ramtree
	sudo mount -oloop ./floppy.386 ./mnt
	cp kern-i386 mnt
	${STRIP} mnt/kern-i386
	cp apps/eve/eve mnt/eve.elf
	${STRIP} mnt/eve.elf
	(cd mnt && du -s * && cd ..) | grep -v lost+found | grep -v boot
	sudo umount ./mnt
	/sbin/e2fsck floppy.386
	touch floppy.386

clean:
	make -C apps clean
	make -C tools/ramtreemaker clean
	find . -name '*.o' -print | xargs rm >/dev/null 2>&1 || true
	find ../dlib -name '*.o' -print | xargs rm >/dev/null 2>&1 || true
	#find . -name '*.a' -print | xargs rm >/dev/null 2>&1 || true
	rm kern-* >/dev/null 2>&1 || true

i386:
	./build-i386

gdb:
	gdb ./kern-i386 :1234

dep:
	makedepend -f Makefile.depends -- ${CFLAGS} -I./include -I./include/* -I./include/*/* -- `find . | egrep '\.[cSh]' | grep -v '\.svn'`

include Makefile.depends
