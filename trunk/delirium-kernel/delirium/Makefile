include Makefile.delirium
include arch/${ARCH}/Makefile.arch

KERNLDFLAGS=-O1 -nostdlib -Map kern-${ARCH}.map --cref -T./kernel-${ARCH}.x

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
KERNOBJS+=kernel/ramtree.o kernel/ktimer.o

ifdef ENABLE_GDB_STUB
KERNOBJS+=kernel/gdb-support.o kernel/gdb-stub.o
endif
ifdef ENABLE_DEBUG_THREAD
KERNOBJS+=kernel/debug.o
endif


default: kern-${ARCH} apps symtab ctags

symtab:
	nm ./kern-${ARCH} | awk '{print $$1 " " $$3}' > syms

ctags:
	ctags -R

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

ramtree-base: ramtreemaker
	tools/ramtreemaker/ramtreemaker ./delirium.ramtree < ramtree.src

ramtree-xen-install: ramtree-base
	cp ./delirium.ramtree boot

ramtree-i386-install: ramtree-base
	cp ./delirium.ramtree boot

# legacy
old-ramtree: ramtree-i386-install

old-ramtree-i386-install: ramtree-base
	sudo mount -oloop ./floppy.386 ./mnt
	cp ./delirium.ramtree mnt
	sudo umount ./mnt
	/sbin/e2fsck floppy.386
	touch floppy.386
	
BOOTTMPDIR=/tmp/deliriumboottmp

image-xen: kern-xen apps ramtree-xen-install
	cp xen.gz boot
	cp kern-xen boot
	echo "(fd0)	`pwd`/floppy.xen" > boot/boot/grub/device.map
	rm -rf $(BOOTTMPDIR) || true
	# There isn't room on the floppy for the svn metadata
	# (not that it should be there anyhow) -- davidb
	#genext2fs -b 1440 -d boot/ ./floppy.xen 
	( if [ -d boot/.svn ]; then svn export boot/ $(BOOTTMPDIR); else	cp -a boot $(BOOTTMPDIR); fi )
	genext2fs -b 1440 -d $(BOOTTMPDIR)/ ./floppy.xen && \
	echo -e 'root (fd0)\ninstall /boot/grub/stage1 d (fd0) /boot/grub/stage2\nquit\n' | /usr/sbin/grub --batch --device-map=`pwd`/boot/boot/grub/device.map --no-floppy --boot-drive='(fd0)' > /dev/null
	rm -rf $(BOOTTMPDIR) || true


image-i386: kern-i386 apps ramtree-i386-install
	echo "(fd0)	`pwd`/floppy.386" > boot/boot/grub/device.map
	rm -rf $(BOOTTMPDIR) || true
	# There isn't room on the floppy for the svn metadata
	# (not that it should be there anyhow) -- davidb
	( if [ -d boot/.svn ]; then svn export boot/ $(BOOTTMPDIR); else	cp -a boot $(BOOTTMPDIR); fi )
	cp kern-i386 $(BOOTTMPDIR)
	cp apps/eve/eve $(BOOTTMPDIR)/eve.elf
	${STRIP} $(BOOTTMPDIR)/kern-i386 $(BOOTTMPDIR)/eve.elf
	cp apps/slip/slip $(BOOTTMPDIR)/slip.elf
	genext2fs -b 1440 -d $(BOOTTMPDIR)/ ./floppy.386 && \
	echo -e 'root (fd0)\ninstall /boot/grub/stage1 d (fd0) /boot/grub/stage2\nquit\n' | /usr/sbin/grub --batch --device-map=`pwd`/boot/boot/grub/device.map --no-floppy --boot-drive='(fd0)' > /dev/null
	rm -rf $(BOOTTMPDIR) || true

deprecated-image-i386: kern-i386 apps ramtree
	sudo mount -oloop ./floppy.386 ./mnt
	cp kern-i386 mnt
	${STRIP} mnt/kern-i386
	cp apps/eve/eve mnt/eve.elf
	${STRIP} mnt/eve.elf
	(cd mnt && du -s * && cd ..) | grep -v lost+found | grep -v boot
	sudo umount ./mnt
	/sbin/e2fsck floppy.386
	touch floppy.386

image: image-${ARCH}

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
	makedepend -f Makefile.depends -- ${CFLAGS} -I./include -I./include/* -I./include/*/* -I./apps/* -I./apps/*/* -- `find . | egrep '\.[cSh]' | grep -v '\.svn' | grep -v ramtreemaker | grep -v dlib/test_`

include Makefile.depends

