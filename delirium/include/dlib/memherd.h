#ifndef __MEMHERD_H
#define __MEMHERD_H
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
 * Copyright (c)2005 David Basden <davidb-delirium@rcpt.to>
 */

#include "bitvec.h"

#ifndef size_t
typedef unsigned int size_t;
#endif
#ifndef NULL
#define NULL ((void *) 0)
#endif

struct herd {
	char *mem;
	size_t mem_size;
	size_t block_size;
	size_t total_blocks;
	size_t free_blocks;
	size_t avail_hint;
	bitvec_t herdvec;
};
typedef struct herd herd_t;


#ifdef __MEMHERD_C
/* Converting offsets to and from blocks */
#define _MEM_TO_OFFSET(_herd, _mem) (((char *)(_mem)) - (_herd)->mem)
#define _OFFSET_TO_MEM(_herd, _off) (((_herd)->mem) + ((size_t)(_off)))

#define _OFFSET_TO_BLOCK(_herd, _off) ((size_t)(_off) / (_herd)->block_size )
#define _BLOCK_TO_OFFSET(_herd, _block) ((size_t)(_block) * (_herd)->block_size )

#define _MEM_TO_BLOCK(_herd, _mem) _OFFSET_TO_BLOCK(_herd, _MEM_TO_OFFSET(_herd, _mem))
#define _BLOCK_TO_MEM(_herd, _block) _OFFSET_TO_MEM(_herd, _BLOCK_TO_OFFSET(_herd, _block))
#endif


/* 
 * Initialise a new memory herd
 */
herd_t new_memherd(void *mem, size_t memsize, size_t blocksize);


/* 
 * Get a block from the herd, and mark it as used.
 * Returns NULL iff no free blocks are available
 */
void *memherd_getBlock(herd_t *h);

/*
 * Return a block to the herd, and mark it as free
 * to be re-allocated
 *
 * pre: memhandle was returned by memherd_getBlock 
 */
void memherd_freeBlock(herd_t *h, void *memhandle);

#endif
