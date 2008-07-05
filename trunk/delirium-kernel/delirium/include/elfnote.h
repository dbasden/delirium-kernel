#ifndef __ELFNOTE_H
#define __ELFNOTE_H

/*
 * Simple Elf Note macros for gas
 *
 * (c)2007 David Basden
 */

#ifdef __ASSEMBLER__

.macro ELFNOTE name, type, desc:vararg
	.p2align 2
	.long	1f - 0f
	.long	3f - 2f
	.long	\type
0:	.asciz	"\name"
1:	.p2align 2
2:	\desc
3:	.p2align 2
.endm

#endif

#endif
