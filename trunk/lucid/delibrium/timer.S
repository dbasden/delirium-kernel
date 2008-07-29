	.globl	rdtsc

	.text
	.align 8

rdtsc:
	// Dump the contents of TSC into %eax and %edx
	rdtsc
	ret
