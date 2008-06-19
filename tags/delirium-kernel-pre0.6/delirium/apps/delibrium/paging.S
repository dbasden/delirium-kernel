	.globl	get_current_page_dir

	.text
	.align 8

get_current_page_dir:
	mov	%cr3, %eax
	ret
