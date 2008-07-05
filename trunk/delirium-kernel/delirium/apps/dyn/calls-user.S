
///////////////////////// 
//
// For crt0.S
//

	.text
	.align 8

.global	mask_interrupt
.global	unmask_interrupt
.global	yield
.global	kill_current_thread
.global	get_thread_info
.global	splinter
.global	add_interrupt_handler
.global	remove_from_pagedir
.global	add_to_pagedir
.global	print
.global	get_new_soapbox
.global	get_soapbox_from_name
.global	believe
.global	renounce
.global	supplicate
.global	get_next_rant
.global	rant
.global	freepages
.global	getpages
.global	herdpages
.global	getpage
.global	freepage
.global	await

.set	mask_interrupt, 0x7ffff000
.set	unmask_interrupt, 0x7ffff004
.set	yield, 0x7ffff008
.set	kill_current_thread, 0x7ffff00c
.set	get_thread_info, 0x7ffff010
.set	splinter, 0x7ffff014
.set	add_interrupt_handler, 0x7ffff018
.set	remove_from_pagedir, 0x7ffff01c
.set	add_to_pagedir, 0x7ffff020
//.set	print, 0x7ffff024
print:	
	jmp 0x7ffff024
.set	get_new_soapbox, 0x7ffff028
.set	get_soapbox_from_name, 0x7ffff02c
.set	believe, 0x7ffff030
.set	renounce, 0x7ffff034
.set	supplicate, 0x7ffff038
.set	get_next_rant, 0x7ffff03c
.set	rant, 0x7ffff040
.set	freepages, 0x7ffff044
.set	getpages, 0x7ffff048
.set	herdpages, 0x7ffff04c
.set	getpage, 0x7ffff050
.set	freepage, 0x7ffff054
.set	await, 0x7ffff058
