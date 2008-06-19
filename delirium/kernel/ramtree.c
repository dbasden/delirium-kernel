#include <delirium.h>
#include "klib.h"
#include "ramtree.h"
#include "paging.h"
#include "soapbox.h"

#define alloca	__builtin_alloca


/* 
 * Convert the relative ramtree addresses into absolute ones and
 * index the ramtree into the soapbox tree.
 *
 * The pointer in the soapbox tree is of type struct ramtree *
 */
void index_ramtree(struct ramtree *rt) {
	int i;

	for (i=0; i < rt->total_entries; i++) {
		
		// Remap to absolute address
		rt->entries[i].start = (void *)rt + (size_t)rt->entries[i].start;
		rt->entries[i].name = (char *)rt + (size_t)(rt->entries[i].name);

		set_soapbox_from_name((soapbox_id_t) &(rt->entries[i]), 
					rt->entries[i].name);
		kprintf("ramtree:\t0x%8x\t%d\t%s\n", rt->entries[i].start,
				rt->entries[i].length, rt->entries[i].name);
	}
}


/*
 * Load ramtree into memory.
 *
 * The data itself is probably already loaded somewhere, but it might be
 * in buffers, or wiped out by a flock of evil geese. Allocate some memory
 * for it, copy it out, and then build the treee to the newly allocated area
 */
void load_ramtree(void *ramtreebase) {
	struct ramtree	*rt;
	void **pages;
	void *imagebase;
	int pages_needed;

	rt = ramtreebase;

	if (*((u_int32_t *)rt) != RAMTREE_MUNDANE) {
		kprint("Invalid header on ramtree\n");
		return;
	}

	pages_needed = rt->total_size / PAGE_SIZE;
	if (rt->total_size % PAGE_SIZE)
		pages_needed++;

	pages = alloca(sizeof(void *) * pages_needed);
	if (kgetpages(pages_needed, pages) != pages_needed)  {
		kprint("Couldn't get enough memory to load ramtree");
		return;
	}
	imagebase = kherdpages(pages_needed, pages);

	kmemcpy(imagebase, ramtreebase, rt->total_size);
	index_ramtree(imagebase);
}
