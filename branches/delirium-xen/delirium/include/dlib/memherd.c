/*
 * Simple memory block allocator
 *
 * Only dependancy is bitvec.h (which itself has no dependencies)
 *
 * Take a large chunk of memory, and split it into
 * blocks of equal size. Allow those blocks to be allocated
 * and deallocated at will.
 *
 * Some uses:
 * 	- Keeping track of free memory frames
 * 	- Keeping of non-trivial ADTs where there isn't a malloc(),
 * 	  but the ADT structures are of a predictable size
 *
 * Not thread-safe for a specific herd, i.e.any given herd can
 * only ever have one operation (new/get/set) running at any
 * given time. Seperate herds do not affect each other.
 *
 * Copyright (c)2005 David Basden <davidb-delirium@rcpt.to>
 */

#include <assert.h>
#include "bitvec.h"

#define __MEMHERD_C
#include "memherd.h"

herd_t new_memherd(void *mem, size_t memsize, size_t blocksize) {
	herd_t newherd;
	u_int32_t *p;
	char *cp;

	newherd.mem = mem;
	newherd.mem_size = memsize;
	newherd.block_size = blocksize;
	newherd.total_blocks = _OFFSET_TO_BLOCK(&newherd, memsize+1);
	newherd.herdvec = mem + memsize - (BITVEC_SIZE(newherd.total_blocks));
	newherd.free_blocks = _MEM_TO_BLOCK(&newherd, newherd.herdvec);
	/* 
	 * Only used total_blocks to allocate herdvec. As we already have mem_size
	 * as a boundry, we don't need it, and it is much more useful to the user
	 * to give it as a high-water mark for free_blocks
	 */
	newherd.total_blocks = newherd.free_blocks;
	newherd.avail_hint = 0;

	/* Set every bit in the vector to free (set) */
	for (p = (u_int32_t *)newherd.herdvec; p < (u_int32_t *)(mem+memsize); p++)
		*p = 0xffffffff;

	/* Set bits for the blocks used by the index as unavailable
	 * (cleared) : TODO: This is REALLY slow and ugly. Fix */
	for (cp = (char *)newherd.herdvec; cp < (char *)(mem+memsize); cp++)
		BITVEC_CLEAR(newherd.herdvec, _MEM_TO_BLOCK(&newherd, cp));

	return newherd;
}

/* Change the hint to the vicinity of a free item in the vector
 *
 * pre: there is a free item in the vector
 */
inline static void bitvec_move_hint(herd_t *h) {
	bitvec_t hitoffset;	
	bitvec_t limit;

	hitoffset = & BITVEC_OFFS(h->herdvec, h->avail_hint);
	limit = (u_int32_t *)((char *)(h->herdvec)) + BITVEC_SIZE(h->total_blocks);

	for (; hitoffset < limit; hitoffset++)  {
		if (*hitoffset){
			h->avail_hint = (hitoffset - (h->herdvec)) * 32;
			return;
		}
	}
	for (hitoffset = (h->herdvec); hitoffset < limit; hitoffset++)  {
		if (*hitoffset) {
			h->avail_hint = (hitoffset - (h->herdvec)) * 32;
			return;
		}
	}
	return;
}

/* Get the next free item from bit vector
 * pre: there /is/ a free item in the vector */
inline static size_t bitvec_get_next_free(herd_t *h) {
	size_t ret;

	if (BITVEC_GET(h->herdvec, h->avail_hint))
		return h->avail_hint;
	bitvec_move_hint(h);
	for (ret = h->avail_hint; ret < h->total_blocks; ret++)
		if (BITVEC_GET(h->herdvec, ret)) return ret;
	for (ret = 0; ret < h->total_blocks; ret++)
		if (BITVEC_GET(h->herdvec, ret)) return ret;
	assert(0);
	return -1;
}

/* 
 * Get a block from the herd, and mark it as used.
 * Returns NULL iff no free blocks are available
 */
void *memherd_getBlock(herd_t *h) {
	size_t block;

	if (!h->free_blocks) return NULL;
	h->free_blocks--;
	block = bitvec_get_next_free(h);
	assert(block < h->total_blocks);
	BITVEC_CLEAR(h->herdvec, block);
	return _BLOCK_TO_MEM(h, block);
}

/*
 * Return a block to the herd, and mark it as free
 * to be re-allocated
 *
 * pre: memhandle was returned by memherd_getBlock 
 */
void memherd_freeBlock(herd_t *h, void *memhandle) {
	size_t block;

	block = _MEM_TO_BLOCK(h, memhandle);
	assert(block < h->total_blocks);
	assert(h->free_blocks < h->total_blocks);
	BITVEC_SET(h->herdvec, block);
	h->avail_hint = block;
	h->free_blocks++;
}

