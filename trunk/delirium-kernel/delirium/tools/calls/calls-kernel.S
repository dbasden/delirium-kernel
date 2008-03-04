
////////////////////////////////////////
//
// Mapping from user #PFs to kernel jmp
//

	.text

	.global	pfhook_base
	.global pfhook_end
	.global pfhook_table

pfhook_base:
	.long	0x7ffff000
pfhook_end:
	.long	0x7ffff058

	.align 8
pfhook_table:
	.long	[pic_mask_interrupt]
	.long	[pic_unmask_interrupt]
	.long	[yield]
	.long	[kill_current_thread]
	.long	[get_thread_info]
	.long	[splinter]
	.long	[add_handler]
	.long	[paging_remove_from_table]
	.long	[paging_add_to_table]
	.long	[vgaprint]
	.long	[get_new_soapbox]
	.long	[get_soapbox_from_name]
	.long	[believe]
	.long	[renounce]
	.long	[supplicate]
	.long	[get_next_rant]
	.long	[rant]
	.long	[kfreepages]
	.long	[kgetpages]
	.long	[kherdpages]
	.long	[kgetpage]
	.long	[kfreepage]
