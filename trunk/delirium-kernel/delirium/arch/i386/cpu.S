/* cpu.S 
 *
 * kernel panics, setup idt, setup tss
 */
#define ASM     1

#include <i386/gdt.h>
#include <i386/interrupts.h>

        .text

	.globl	kpanic, khalt, meditate, _idt_base, 
	.globl	_idt_interrupt_template, setup_idt
	.globl	save_first_tss, memcpy, backtrace
	.globl	get_eflags

	
meditate:
	pushl	vga_print_s
	pushl	%ebp
	mov	%esp, %ebp
	cli
	call backtrace
	pusha	// EAX ECX EDX EBX ESP EBP ESI EDI
	pushf
	pushl	%gs
	pushl	%fs
	pushl	%es
	pushl	%ds
	pushl	%ss
	pushl	%cs
	pushl	$(med_string)
	movl	$0x1, vga_print_s
	call	kprintf
	popl	vga_print_s
	mov	%ebp, %esp
	popl	%ebp
	ret

kpanic:
	cli
	call backtrace
	pusha	// EAX ECX EDX EBX ESP EBP ESI EDI
	pushf
	pushl	%gs
	pushl	%fs
	pushl	%es
	pushl	%ds
	pushl	%ss
	pushl	%cs
	pushl	$(med_string)
	movl	$0x1, vga_print_s
	call	kprintf
	addl	$0x40, %esp
khalt:
0:
	pause
#ifndef	ENABLE_BOCHS_HACK
	hlt
#endif
	jmp 0b

get_eflags:
	pushf
	popl	%eax
	ret

/*
 * Evil hack (why is this here?!)
 */
memcpy:
	jmp kmemcpy

backtrace:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	vga_print_s
	movl	$0x1,vga_print_s
	pushl	36(%ebp)
	pushl	32(%ebp)
	pushl	28(%ebp)
	pushl	24(%ebp)
	pushl	20(%ebp)
	pushl	16(%ebp)
	pushl	$(bt_string)
	call	kprintf
	#if 0
	addl	28, %esp
	popl	vga_print_s
	#endif
	movl	%ebp, %esp
	popl	%ebp
	ret


setup_idt:
	lidt	_idt_ptr
	ret

save_first_tss:
	pushl	%ebp		
	movl	%esp, %ebp
	pushal	// EAX ECX EDX EBX ESP EBP ESI EDI
	popl	(init_tss + 68)
	popl	(init_tss + 64)
	popl	(init_tss + 60)
	popl	(init_tss + 56)
	popl	(init_tss + 52)
	popl	(init_tss + 48)
	popl	(init_tss + 44)
	popl	(init_tss + 40)

	pushf
	popl	(init_tss + 36)
	movl	%ebp, (init_tss + 4) // esp0
	movl	%ebp, (init_tss + 12) // esp1
	movl	%ebp, (init_tss + 20) // esp2
	movl	%ebp, (init_tss + 24)
	movl	%ebp, %esp
	popl	%ebp
	ret

kp_string:
	.asciz	"Kernel panic\n"
bt_string:
	.ascii	"\n0x%000008x (esp)	stack backtrace\n"
	.ascii	"-------------------\n"
	.ascii	"0x%000008x (esp+4)\n"
	.ascii	"0x%000008x (esp+8)\n"
	.ascii	"0x%000008x (esp+12)\n"
	.ascii	"0x%000008x (esp+16)\n"
	.asciz	"0x%000008x (esp+20)\n"
med_string:
	// A small kitten rests
	// to meditate on the day;
	// Let us join it now.
	.ascii	"\n" 
	.ascii	"Storm sleeps peacefully,               cs %000008x   ss %000008x   ds %000008x\n"
	.ascii	"Caring not for today's trials;         es %000008x   fs %000008x   gs %000008x\n"
	.ascii	"Happy dreams abound.                   eflags %000000000000000000000000008888b\n"
	.ascii	"                        edi %000008x  esi %000008x  ebp %000008x  esp %000008x\n"
	.asciz  "          ebx %000008x  edx %000008x  ecx %000008x  eax %000008x  eip %000008x"

	.align	8
_idt_interrupt_template:
	.word	0		/* Offset bits 0-15 */
	.word	0x0008	// GDT_KERNEL_CODE	/* segment selector */
	.word	0x8e00		/* flags dpl  INTERRUPT_GATE | SEG_PRESENT */
	.word	0		/* offset 16-31 */

	.align	8
	.word	0
_idt_ptr:
	.word	0x0600	/* 256 entries at 6 bytes a pop */
	.long	[_idt_base]

	.align	8
_idt_base:
	.fill	256,	8
