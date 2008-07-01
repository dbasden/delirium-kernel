#define	ASM
#include "i386/interrupts.h"

	.text

	.globl	_start, exit, end

_start:
#if 0
	call getpage
	addl $4096, %eax
	mov %eax, %esp
	mov %esp, %ebp
#endif
	call dream
end:
exit:
	int $IRQ_KILL
	int $IRQ_YIELD
	jmp exit
