/* inth.S - Interrupt Handlers */

	.text
	.globl	do_context_switch, default_interrupt_handler
	.globl  abort_handler, trap_handler, fault_handler
	.globl	inth_timer, inth_kill_current_thread, tempstack


/*
 * Push_Context, Switch_Context and Pop_Context
 *
 * Taken together, these 3 macros are do_context_switch
 */

	// We put the following on the thread's own stack when switching out of a context:
	//   - EFLAGS, EIP and CS are already on stack (through software interrupt)
	//   - EAX ECX EDX EBX ESP EBP ESI EDI through PUSHA
	//   - DS, and SS
	//
	// When switching, from T(i) to T(j), the state for T(i) is just saved on it's own
	// stack, just like any other interrupt. We can wimp out and actually
	// save this into protected RING 0 memory later to avoid stuff like CS
	// and EFLAGS being changed in a lower protection level by another
	// thread in the same program
	//
	.macro Push_Context
	push %ds
	push %ss
	pusha	// Add EAX ECX EDX EBX ESP EBP ESI EDI
	.endm

	.macro Switch_Context
	movl	%esp, %eax
	push 	%eax
	call	get_next_switch

	/* Returned should be the SP for the new task, with context on stack */
	movl	%eax, %esp	
	.endm

	/* Alternate for Switch_Context is to kill the switched out thread */
	.macro Switch_Kill
	movl	%esp, %eax
	push 	%eax
	call	kill_current_thread
	movl	%eax, %esp	
	.endm

	//
	// All the info we want to save from the previous task is now
	// on the stack. Change EBP to here, and pass it to
	// get_next_switch
	// 
	.macro Pop_Context
	popa		// Restore EAX ECX EDX EBX ESP EBP ESI EDI
	pop %ss		// Restore stack sement
	pop %ds		// Restore data segment
	.endm

	.macro EOI_pic_lo
	mov 	$0x20,	%al
	outb	%al,	$0x20
	.endm

	.macro EOI_pic_hi
	mov 	$0x20,	%al
	outb	%al,	$0x20
	outb	%al,	$0xa0
	.endm

	.macro HW_Int_Hook inum
	.text
isr_hook_\inum:	.globl isr_hook_\inum
	Push_Context
	mov (C_isr_p + (\inum * 4)), %eax
	call *%eax
	Pop_Context
	.if (\inum < 8)
	EOI_pic_lo
	.else
	EOI_pic_hi
	.endif
	iret
	.endm

	
do_context_switch: 	/* Software vector handler for requested context switch (YIELD) */
	cli
	Push_Context
	Switch_Context
	Pop_Context
	iret		// Restore EFLAGS, CS and EIP, and reenable interrupts

inth_kill_current_thread:
	cli
#if 0
	// This was probably an earlier attempt at stopping 
	// a triple-fault if the CPU context was all over the place
	// This shoudl now be handled before this point
	mov	%esp, %ebp
	mov 	$(tempstack + 0x1000),	%esp
	pushl	8(%ebp)
	pushl	4(%ebp)
#endif
	Push_Context
	Switch_Kill
	Pop_Context
	iret

inth_timer:		/* Handle timer interrupt */
	Push_Context
	Switch_Context
	EOI_pic_lo
	Pop_Context
	iret


default_interrupt_handler:
	Push_Context	 // Save the context! Collect the whole set!

	mov	$0x10, %ax	// Enforce kernel DS and SS
	mov	%ax, %ds
	mov	%ax, %ss

	pushl	%ebp
	movl	%esp, %ebp
	subl	$0x0c, %esp

	pushl	0x1c(%ebp)
	pushl	0x19(%ebp)
	pushl	$(ih_string)
	call	kprintf
	addl	$0x0c, %esp

	call	kvga_spin

	movl	%ebp, %esp
	popl	%ebp

	Pop_Context
	iret

// Stubs to hook ISRs into C
	HW_Int_Hook 0
	HW_Int_Hook 1
	HW_Int_Hook 2
	HW_Int_Hook 3
	HW_Int_Hook 4
	HW_Int_Hook 5
	HW_Int_Hook 6
	HW_Int_Hook 7
	HW_Int_Hook 8
	HW_Int_Hook 9
	HW_Int_Hook 10
	HW_Int_Hook 11
	HW_Int_Hook 12
	HW_Int_Hook 13
	HW_Int_Hook 14
	HW_Int_Hook 15

	/* table of pointers to the C ISR wrapper hooks */
	.globl c_isr_wrapper_table
        .align 8
c_isr_wrapper_table:
	.long	[isr_hook_0]
	.long	[isr_hook_1]
	.long	[isr_hook_2]
	.long	[isr_hook_3]
	.long	[isr_hook_4]
	.long	[isr_hook_5]
	.long	[isr_hook_6]
	.long	[isr_hook_7]
	.long	[isr_hook_8]
	.long	[isr_hook_9]
	.long	[isr_hook_10]
	.long	[isr_hook_11]
	.long	[isr_hook_12]
	.long	[isr_hook_13]
	.long	[isr_hook_14]
	.long	[isr_hook_15]

	//---------------------------
	// When the bad things happen
	//---------------------------
	.macro HandlerSafety
	cli
	mov	%esp, %eax
	movl 	$(tempstack + 0x0a00), %esp
	pushl	%esp	// Old ESP
	pushl	%ebp	// Old EBP
	Push_Context
	.endm

abort_handler:
	HandlerSafety
	pushl $(abort_string)
	jmp 1f
trap_handler:
	HandlerSafety
	pushl $(trap_string)
	jmp 1f
fault_handler:
	HandlerSafety
	pushl $(fault_string)
1:
	call vgaprint_cli
0:
	jmp 0b
	//jmp	meditate
	int	$0x43


	.data
	.globl C_isr_p
	.comm	C_isr_p, (256 * 4)

ih_string:
	.asciz	"Interrupt handler called: 0x%08x 0x%08x\n"
abort_string:
	.asciz	"Default abort handler called.\n"
trap_string:
	.asciz	"Default trap handler called.\n"
fault_string:
	.asciz	"Default fault handler called.\n"


	/* Allocate a page so that we can kill off threads.
	 * WARNING: Is now used for more stuff than this. Really should allocate at the
	 * time unless there is a REALLY good reason (i.e. we have NO stack). tempstack
	 * is blown away by something I suspect because it's overused.
	 */
	.comm	tempstack, 0x1000, 0x1000
