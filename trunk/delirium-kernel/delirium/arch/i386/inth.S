/* inth.S - Interrupt Handlers */

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

.macro pic_EOI_master
		mov	$0x20, %al
		outb	%al, $0x20
.endm

.macro pic_EOI_slave_and_master
		mov	$0x20,	%al
		outb	%al,	$0xa0
		outb	%al,	$0x20
.endm

	//
	// shims to allow hardware interrupts to be switched in C
	//
.altmacro
.macro	MakeISRshim	vec=0,lastvec=15
	.globl isr_shim_\vec
	.align	8
isr_shim_\vec:
	Push_Context

	pushl	$\vec
	call	handle_shimmed_isr
	addl	$4, %esp


	.if (\vec < 8)
	pic_EOI_master
	.else
	pic_EOI_slave_and_master
	.endif

	Pop_Context

	iret

	.if (\vec-\lastvec)
		MakeISRshim %(\vec+1), \lastvec
	.endif
.endm

	//
	// actual code starts here
	//

	.text

	.align	8

	.globl	do_context_switch
	.globl  abort_handler, trap_handler, fault_handler
	.globl	early_timer_isr, inth_timer
	.globl	inth_kill_current_thread

	//
	// ISR shims
	//
	MakeISRshim	0,15


	#ifdef TWO_YEAR_OLD
	//
	// Contribution from my daughter
	//		 -- DGB Jul08 
                     7hjkhggkj
                     
                     
                     
                     
                     kvkkkkjhjjtj555555555555224444444uuuuuuuuuuuuiuuuuun99o 05555555555555555555555555555555555555555555555555555555555555555555555555555555555444444444444444444	3333333333333312111806877/
                     yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyttttttttttttttttttttttttttttttttttttttttttrrrrtd6yrmhty8enmborocjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjkkkkllp[]\\\//'//////////.....,,,,,,j6urtgnv hb eeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrrrrrrrrrrrr22o,cv  n y  nr7ybyfyynnghhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj,,,,u7e6675757722222222222222223222277777777777733333333333333333333333333111111````````00000000000000000000000iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiggggffffffdddddbbbbbbbbaaaaaababbababababaaaaaassssszxxxxxxxxxxxxxzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzllllllkkkjjjjjkiiiiuuuupooooopoQA	QAAAqaAQAQwwwwwWWWWQWQA
       #endif

do_context_switch: 	/* Software vector handler for requested context switch (YIELD) */
	cli
	Push_Context
	Switch_Context
	Pop_Context
	iret		// Restore EFLAGS, CS and EIP, and reenable interrupts

inth_kill_current_thread:
	cli
	Push_Context
	Switch_Kill
	Pop_Context
	iret

early_timer_isr:	/* Handle timer interrupt before task switching */
	Push_Context
	call ktimer_on_timer_tick
	pic_EOI_master
	Pop_Context
	iret

inth_timer:		/* Handle timer interrupt */
	Push_Context
	call ktimer_on_timer_tick
	Switch_Context
	pic_EOI_master
	Pop_Context
	iret


.macro HandlerSafety
	// possibly overkill
	cli
	mov	%esp, %eax
	movl 	$(tempstack + 0x0ff0), %esp
	pushl	%eax	// Old ESP
	pushl	%ebp	// Old EBP
	Push_Context
	mov	%esp, %ebp
.endm

abort_handler:
	HandlerSafety
	pushl $(abort_string)
	jmp 1f
trap_handler:
	HandlerSafety
	pushl $(trap_string)
1:
	call	vgaprint_cli
	int	$0x43	// kill current thread

fault_handler:
	cli
	pushl	%ebp
	movl	%esp,%ebp
	pushl	running_thread_id
	pushl	4(%ebp)
	pushl	$(fault_string)
	call	kprintf
	int	$0x43	// kill current thread

	.data

abort_string:
	.asciz	"Default abort handler called.\n"
trap_string:
	.asciz	"Default trap handler called.\n"
fault_string:
	.asciz	"Uncaught fault #%d. Killing thread %d\n"


	/* Allocate a page so that we can kill off threads.
	 * WARNING: Is now used for more stuff than this. Really should allocate at the
	 * time unless there is a REALLY good reason (i.e. we have NO stack). tempstack
	 * is blown away by something I suspect because it's overused.
	 */
	.comm	tempstack, 0x1000, 0x1000
