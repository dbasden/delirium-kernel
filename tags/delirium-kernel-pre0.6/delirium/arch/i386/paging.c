/*
 * IA32 paging for delirium
 *
 * (c)2005 David Basden <davidb-delirium@rcpt.to>
 *
 * All pages in the page table are allocated from the frameherd.
 * They are all hardwired into memory, so the tables point to
 * the same place if paging is switched on or off. This means
 * we can't swap them out, but for now it's easier.
 */
#include <delirium.h>

#include "paging.h"
#include "frameherd.h"
#include "klib.h"
#include "kvga.h"

#include "i386/paging.h"
#include "i386/interrupts.h"

page_table_t	kernel_page_dir;

/*
 * Keep a spare page table allocated, and paged in (but not referenced
 * by any page directory). This way we should never find ourselves in 
 * the unenviable position of having to allocate a new pagetable, and
 * first have to page it in through the self-same pagetable before 
 * being able to put it in the page directory.
 */
page_table_t	spare_page_table;

void paging_add_to_dir(page_table_t dir, void *logical, void *physical);

static inline page_table_t new_page_table() {
	page_table_t pt;

	pt = spare_page_table;

	if (take_from_herd(1, &spare_page_table) != 1) {
		kprint("\nCouldn't allocate a frame for page table!\n");
		kpanic();
	}
#ifdef DEBUG
	kprintf("new pagetable is at 0x%x\n", pt);
#endif

	kmemset((void *)pt, 0, PAGE_SIZE); // fill with zeros!

	return pt;
}

/* index into page directory is bits 22-31 of the logical address */
#define DIR_INDEX(_memaddr)		((size_t) ((size_t)(_memaddr) >> 22))

/* index into page table is bits 12-21 of the logical address */
#define TABLE_INDEX(_memaddr)		( ((size_t) ((size_t)(_memaddr) >> 12)) & ((1 << 12) - 1))

#define FRAME_PHYS(_memaddr)		(((_memaddr) >> 12) << 12)

/*
 * add a frame to a pagetable
 */
void paging_add_to_table(page_table_t table, void *logical, void *physical) {
	table[TABLE_INDEX(logical)] = (u_int32_t) physical | PT_PRESENT, PT_RW;
}

/*
 * add a frame to a pagedir, adding a pagetable if needed
 */
void paging_add_to_dir(page_table_t dir, void *logical, void *physical) {
	if (! dir[DIR_INDEX(logical)]) {
		void *newtable;

		newtable = new_page_table();
		dir[DIR_INDEX(logical)] = (u_int32_t) newtable |
					PT_PRESENT, PT_RW;	

		paging_add_to_dir(kernel_page_dir, spare_page_table, spare_page_table);
		if (dir != kernel_page_dir) paging_add_to_dir(dir, 
				spare_page_table, spare_page_table);
	}


	paging_add_to_table((void *) FRAME_PHYS(dir[DIR_INDEX(logical)]), logical, physical);
}

void paging_remove_from_table(page_table_t table, void *logical) {
	table[TABLE_INDEX(logical)] = 0;
}

void paging_remove_from_dir(page_table_t dir, void *logical) {
	if (dir[DIR_INDEX(logical)]) {
		paging_remove_from_table((void *) FRAME_PHYS(dir[DIR_INDEX(logical)]), logical);
	}
}

/*
 * pre: frameherd is running and has memory map correctly added
 * pre: only memory allocated by the frameherd is for kernel space
 */
void setup_paging() {
	void ** templist;
	int fc;
	
	/* Get a 'spare' pagetable */
	if (take_from_herd(1, &spare_page_table) != 1) {
		kprint("\nCouldn't allocate a frame for page table!\n");
		kpanic();
	}

	/*
	 * Page directory must be 4k, and must be frame aligned
	 */
	kernel_page_dir = new_page_table();

	/* 
	 * In the interest on at least getting to the #PF handler, lets put it in memory, along
	 * with the stack and the VGA textbuffer 
	 */
	paging_add_to_dir(kernel_page_dir, &handle_pf, &handle_pf);
	paging_add_to_dir(kernel_page_dir, &fc, &fc);	// Evil way of getting stack frame
	paging_add_to_dir(kernel_page_dir, VGA_BASE, VGA_BASE);

	templist = (void **) new_page_table();
	fc = get_allocated_frames(PAGE_SIZE / sizeof(void *), templist);
	if ( fc >= (PAGE_SIZE / sizeof(void *))) {
		kprint("More than 4MB of allocated frames already... Unimplemented, so dying.\n");
		// TODO: Fix. All have to do is modify get_allocated_frames so you can skip some
		kpanic();
	}

	for (fc--;fc >= 0; fc--)
		paging_add_to_dir(kernel_page_dir, templist[fc], templist[fc]);


	paging_remove_from_dir(kernel_page_dir, templist);
	return_to_herd(1, &templist);

	add_handler(14, &handle_pf);
	enable_paging();	
}

