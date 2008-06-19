/*
 * arch/xen/boot.S - bootstrap DeLiRiuM through Xen 
 *
 * Copyright (c)2007 David Basden --  All Rights Reserved
 */

#define __ASSEMBLY__
#include <elfnote.h>

#include "xen/public/xen.h"
#include "xen/public/elfnote.h"

/* Memory map for Garden of Eden / 'Start' of day */
#define STACK_SIZE	0x4000

	.bss
	.align 4096
	.globl	hypercall_page
hypercall_page:
HYPERCALL_BASE = .
	.skip 4096

/* Export hypercall offset symbols */
#include "hypercall_gate.S"

/*
 * Start and end locations for the kernel load, so that the frameherd 
 * knows where we are loaded if our bootstrapper can't tell us. These
 * are placed at the beginning and end of the loaded image by the
 * linker script (kern-$ARCH.x).
 *
 * Leave this in, even if we're sure our bootloader will be sane; The
 * bootloader might have bugs, or the memory map may be... eclectic -- DGB
 */
	.section	.delirium_kernel_start
	.align	16
	.globl	delirium_start_ptr
delirium_start_ptr:
	.long	.

	.section	.delirium_kernel_end
	.align	16
	.globl	delirium_end_ptr
delirium_end_ptr:
	.long	.

/*
 * Hints to Xen builder that we are a guest OS kernel image
 * and how we need to be para-virtualised
 */
	.section ".note.Xen", "a"

	ELFNOTE	Xen XEN_ELFNOTE_XEN_VERSION	.asciz "xen-3.0"
	ELFNOTE	Xen XEN_ELFNOTE_GUEST_OS	.asciz "DeLiRiuM"
	ELFNOTE	Xen XEN_ELFNOTE_GUEST_VERSION	.asciz "0.6-xen"
	ELFNOTE	Xen XEN_ELFNOTE_LOADER		.asciz "generic"
	ELFNOTE	Xen XEN_ELFNOTE_VIRT_BASE	.long 0
	ELFNOTE	Xen XEN_ELFNOTE_ENTRY		.long [_start]
	ELFNOTE Xen XEN_ELFNOTE_FEATURES	.asciz "writable_page_tables|auto_translated_physmap|writable_descriptor_tables|"
	ELFNOTE	Xen XEN_ELFNOTE_PAE_MODE	.asciz "yes"
	ELFNOTE	Xen XEN_ELFNOTE_HYPERCALL_PAGE	.long HYPERCALL_BASE

/***********************************/

.text
	.globl	start,_start

	.align	8
start:
_start:
	mov	$(kstack + STACK_SIZE), %esp	// Initialise the stack pointer

	movl	$(CONSOLEIO_write), %ebx
	movl	$80, %ecx
	movl	$initmsg_a, %edx
	call	(hypercall_page + ( __HYPERVISOR_console_io * 32 ))

	/* Push empty multiboot information */
	pushl	0
	pushl	0

	/* C entry point*/
        call    cmain

	pushl	$halt_message
	call	kprint

	/* Xen will really spew at this */
	hlt

	/* And just in case... */
	jmp	khalt

	/* 
	 * Stacktastic! Align on 4096 byte page
	 */
	.globl	kstack
        .comm   kstack, STACK_SIZE, 4096

	.text
	/*
	 * Keep these messages in the text section, so they should be in the same
	 * frame as the code that calls them; Early on we don't assume that any other
	 * frames are mapped in, and if something goes wrong, it's more likely that
	 * the same frame is going to be available (if anything is)
	 */
        .text
halt_message:
        .asciz  "Errrk. Kernel returned! Bored now."
initmsg_a:
	.asciz	"-----------------------------------> DeLiRiuM/Xen... <--------------------------\n"
initmsg_b:
	.asciz	"Bootstrapped\n"

