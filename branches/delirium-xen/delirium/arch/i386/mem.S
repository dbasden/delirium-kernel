#define ASM	1

/* mem.S - set up delirium memory management */
#include <delirium.h>
#include "i386/task.h"
        
        .text

        .globl  install_gdt
	.globl	gdt_descriptor
	.globl	gdt_entries
	.globl	init_tss
	.globl	install_init_tr
	.globl	set_pdbr
	.globl	enable_paging
	.globl	disable_paging
	.globl	handle_pf

	.align 4

install_gdt:
	mov	$0x00000010,	%eax
	lgdt	gdt_descriptor
	mov	%ax,	%ss
	mov	%ax, 	%ds
	mov	%ax, 	%es
	mov	%ax, 	%fs
	mov	%ax, 	%gs
	ljmp	$8,	$fnord
fnord:
	ret

install_init_tr:
	mov	$0x0018 ,%ax
	ltr	%ax
	ret

set_pdbr:
	mov	4(%esp), %eax 
	mov	%eax, %cr3
	ret

enable_paging:
	// Set bit 31 of CR0
	//
	movl	%cr0, %eax
	bts	$31, %eax
	movl	%eax, %cr0
	ret

disable_paging:
	movl	%cr0, %eax
	btr	$31, %eax
	movl	%eax, %cr0
	ret

handle_pf:
	// Page Fault handler. Cannot assume:
	//	-- Correct segments
	//	-- Existence of stack

	// Check to see if a dummy address is being called. If it is,
	// remap the call to the real function address.
	//
	movl 	%cr2, %ecx
	cmpl	pfhook_base, %ecx
	jb	9f
	cmpl	pfhook_end, %ecx
	ja	9f
	subl	pfhook_base, %ecx
	addl	$(pfhook_table), %ecx
	addl	$0x4, %esp
	movl	(%ecx), %eax
	movl	%eax, (%esp)
	iret
9:
chip_tax:
	cli
	// Let's assume we can still get to video memory
	// Write PF on the  screen
	movl	$0x70467050, 0xb8000+3836
	// Can't call disable paging if the stack is fubared
	// Do it manually
	//call	disable_paging
	movl	%cr0, %eax
	btr	$31, %eax
	movl	%eax, %cr0
	// Save the stack pointer, and use the emergency stack
	movl	%ebp, %eax
	movl	%esp, %ebp
	movl	$(emergency_stack + 0x1000), %esp
	pushl	%eax	// Old EBP
	pushl	%ebp	// Old EAX
	movl	%esp, %ebp
	// Print a handy error message
	pushl	running_thread_id
	pushl	12(%ebp)
	pushl	%ecx
	pushl	$(pf_message)
	movl	$1, vga_print_s
	call 	kprintf
	addl	$24, %esp
	// Make sure that the running thread isn't the kernel
	// thread. If it is kpanic instead of just killing the thread
	cmpl	$0, running_thread_id
	jne	8f
	pushl	$(pf_message)
	jmp	kpanic
8:
	// Uncomment to warn that we are going to switch on paging
	//pushl	$(paging_on_msg)
	//call	vgaprint_cli

	// Reenable paging. We have our super-happy-emergency-stack
	// by this point, so 'call' should work
	call	enable_paging
	//pushl	$(paging_on_works_msg)
	//call	vgaprint_cli

	// We're still in the 'interrupt' context. If we jmp
	// to the handler for kill_current_thread, it should just
	// work properly, and then iret back while switching context.
	// The thread when killed shuold no longer be using the emergency
	// stack. If it does, we are kinda screwed anyhow.

	// pretend that we are a coming from an interrupt context
	// Have "dont_go_there" as the EIP so that we can notice if debugging
	//
	pushl	dont_go_there
	pushf
	pushl	%cs
	pushl	dont_go_there
	jmp	inth_kill_current_thread
dont_go_there:
	.globl	dont_go_there
	jmp kpanic

        
pf_message:
	.asciz	"#PF accessing [0x%8x] by [0x%8x]. Killing thread %d.\n"
paging_on_msg:
	.asciz	"About to re-enable paging after massive insanity fest. This might break.\n"
paging_on_works_msg:
	.asciz	"Well, got this far... Now trying to kill the current thread\n"


	.align 8
	.word	0	/* correct for gdt descriptor is addr mod 4 == 2 */
gdt_descriptor:
	.word (8 * 4)	/* 4 entries, 8 bytes each */
	.long [gdt_entries]
        
	.align 8	/* correct alignment for gdt is mod 8 == 0 */
gdt_entries:
	.long 0 /* GDT Sentinel */
	.long 0

	/* index 0x1 (ljmp $0x0008, $nextline)
	 * KERNEL CS - whole 4GB
	 */
	.word 0xffff		/* limit bits 0-15 */
	.word 0		/* base bits 0-15 */
	.byte 0		/* base bits 0-23 */
	.byte 0x9a	/* access flags : Present, DPL 0, Code R/W */
	.byte 0xcf	/* GDP flags and limit bits 16-19: Paged, 32bit, 0xf */
	.byte 0		/* base bits 24-31 */
	
	/*
	 * index 0x2 (mov 0x0010 ds)
	 * Kernel DS - whole 4GB
	 */
	.word 0xffff	/* limit bits 0-15 */
	.word 0		/* base bits 0-15 */
	.byte 0		/* base bits 0-23 */
	.byte 0x92	/* access flags : Present, DPL 0, Data R/W*/
	.byte 0xcf	/* GDP flags and limit bits 16-19 */
	.byte 0		/* base bits 24-31 */

	/*
	 * index 0x3: TSS
	 */

	.fill 4, 8

	/* TSS */
	.comm	init_tss, 104

	/* Emergency stack: only for use by #PF handler */
	.align 8
	.comm	emergency_stack, 0x1000
