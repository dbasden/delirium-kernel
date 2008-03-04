#define	ASM
#include "i386/interrupts.h"

	.text

	.globl	_start, exit, end

_start:
	call getpage
	addl $4096, %eax
	mov %eax, %esp
	mov %esp, %ebp
	call dream
	call thread_ret
end:
exit:
	//int $IRQ_KILL
	int $IRQ_YIELD
	jmp exit
