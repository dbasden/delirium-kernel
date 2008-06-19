#ifndef __FRAMEHERD_H
#define __FRAMEHERD_H
/*
 * Simple kernel frame allocator for memory in deLiRiUM
 *
 * David Basden <davidb-delirium@rcpt.to>
 */

#include <delirium.h>
#include "dlib/bitvec.h"

#define FRAME_SIZE	4096

#define _MEM_TO_FRAME(_mem)	((size_t)( _mem) / FRAME_SIZE)
#define _FRAME_TO_MEM(_frame)	((void *) (FRAME_SIZE * (_frame)))

/* move to klib? */
#define MEGABYTE	1048576
#define GIGABYTE	1073741824

/* Change to relocate frame index */
#define HERD_BASE	0x6000	

/* Change to increase mem size */
#define HERD_FRAMES	(_MEM_TO_FRAME(GIGABYTE)) 

#define HERD_INDEX_SIZE		BITVEC_SIZE(HERD_FRAMES)

void init_herder();

/*
 * allocate up to frames frames from the pool
 *
 * addrs is a list of void pointers, with at least frames entries
 * the base addresses of the frames will be stored there
 *
 * returns the amount of frames allocated
 */
int take_from_herd(int frames, void *addrs);

/*
 * free the frames at given addresses to be available again
 */
void return_to_herd(int frames, void *addrs);

/*
 * flag memory as already allocated
 *
 * Any frame that contains memory in this area will be flagged
 * as allocated, not just the memory itself.
 *
 * WARNING: This call does NO checking at all, and should 
 * only be used when setting up memory at kernel entry, before
 * any frames are requested from the herder. Otherwise, the
 * frames reserved here may be (legitimately) used by the process
 * that thinks it has a lock on it.
 *
 * It's also REALLY slow.
 */
void take_mem_from_herd(void *base, size_t extent);

/*
 * as per take_mem_from_herd, but flags memory as available
 *
 * The same inclusiveness applies. ALL frames containing even a byte of
 * the memory will be available. Be VERY careful when doing this
 */
void add_mem_to_herd(void *base, size_t extent);


int get_allocated_frames(int, void *);

#endif
