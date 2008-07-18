/*
 * Simple kernel frame allocator for memory in deLiRiUM
 *
 * NOT thread safe!
 *
 * David Basden <davidb-delirium@rcpt.to>
 */

#include <delirium.h>
#include <assert.h>
#include <cpu.h>

#include "dlib/bitvec.h"
#include "frameherd.h"
#include "klib.h"
#include "paging.h"

#define _MEM_TO_FRAME(_mem)	((size_t)( _mem) / FRAME_SIZE)
#define _FRAME_TO_MEM(_frame)	((void *) (FRAME_SIZE * (_frame)))

/* klib? */
#define MEGABYTE	1048576
#define GIGABYTE	1073741824

static volatile Semaphore INIT_SEMAPHORE(frameherd_s);

/* 
 * Change to relocate frame index.
 *
 * Note: This is pretty much the only part of memory that is
 *	 not handled dynamically apart from kernel entry.
 *
 * TODO: Should probably move it to be allocated by the ELF loader, 
 *       just like everything else is.
 */
#define HERD_BASE	0x6000	

/* Change to increase mem size */
#define HERD_FRAMES	(_MEM_TO_FRAME(GIGABYTE)) 

/* Pointer to the herd base */
static bitvec_t volatile	herd = (void *) HERD_BASE;



/*
 * Physical memory map cache
 *
 * We cache the locations of physical memory that have been
 * added to the frameherder through add_mem_to_herd. This
 * annoyingly limits the amount of non-contiguous blocks
 * handled by the herder to a pathetically hardcoded amount.
 */

#define MAX_MAP_CACHE_ENTRIES	32

struct cache_entry {
	void 	*base;
	size_t	bytes;
};

static volatile struct cache_entry	map_cache[MAX_MAP_CACHE_ENTRIES];
static volatile size_t			map_cache_entries = 0;


void init_herder() {
	/* No Free Frames */
	kmemset((void *) herd, 0, BITVEC_SIZE(HERD_FRAMES));	
}

/*
 * return the next frame free in the pool given
 *
 * returns -1 iff there are no free frames in the pool
 */
inline static int next_free_in_pool() {
	int i;

	// TODO: remove BAD O(n)
	for (i=0; i < HERD_FRAMES; i++) 
		if (BITVEC_GET(herd, i))
				return i;

	return -1;
}


/*
 * allocate up to frames frames from the pool
 *
 * addrs is a list of void pointers, with at least frames entries
 * the base addresses of the frames will be stored there
 *
 * returns the amount of frames allocated
 */
int take_from_herd(int frames, void *addrs) {
	int taken;
	int f;

	if (LOCKED_SEMAPHORE(frameherd_s) && (! _INTERRUPTS_ENABLED())) {
		kprint("take_from_herd: Deadlock! frameherd mutex semaphore taken and interrupts are off!\n");
		kpanic();
	}
	while (LOCKED_SEMAPHORE(frameherd_s))
		;
	SPIN_WAIT_SEMAPHORE(frameherd_s);
	for (taken = 0; taken < frames; taken++) {
		if ((f = next_free_in_pool()) == -1)
			break;

		BITVEC_CLEAR(herd, f);
		*((void **) addrs) = _FRAME_TO_MEM(f);

		addrs = addrs + sizeof(void *);
	}
	RELEASE_SEMAPHORE(frameherd_s);

	return taken;
}


/*
 * free the frames at given addresses to be available again
 *
 * O(n) in frames
 */ 
void return_to_herd(int frames, void *addrs) { 

	if (LOCKED_SEMAPHORE(frameherd_s) && (! _INTERRUPTS_ENABLED())) {
		kprint("return_to_herd: Deadlock! frameherd mutex semaphore taken and interrupts are off!\n");
		kpanic();
	}
	SPIN_WAIT_SEMAPHORE(frameherd_s);
	while (frames--) { 
	#if 0
		if ( BITVEC_GET(herd, _MEM_TO_FRAME( *((u_int32_t *) addrs)) ) ) {
			kprintf("%s: double free! 0x%8x\n", __func__, *(u_int32_t *)addrs);
			kpanic();
		}
	#endif
		BITVEC_SET(herd, _MEM_TO_FRAME( *((u_int32_t *) addrs)) ); 
		addrs = addrs + sizeof(void *);
	}
	RELEASE_SEMAPHORE(frameherd_s);
}

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
 * It's also REALLY slow: (O(n) in extent)
 */
void take_mem_from_herd(void *base, size_t extent) {
	int i;
	
	for (
			i=(_MEM_TO_FRAME(base)); 
			i <= (_MEM_TO_FRAME(base + extent - 1));
			i++) {
		BITVEC_CLEAR(herd, i);
	}
}

/*
 * as per take_mem_from_herd, but flags memory as available.
 * ALSO records the blocks of memory in a local table.
 *
 * The same inclusiveness applies. ALL frames containing even a byte of
 * the memory will be available. Be VERY careful when doing this
 *
 * O(n) in extent
 *
 */
void add_mem_to_herd(void *base, size_t extent) {
	int i;
	
	Assert(map_cache_entries < MAX_MAP_CACHE_ENTRIES);
	map_cache[map_cache_entries].base = base;
	map_cache[map_cache_entries++].bytes = extent;

	for (i=_MEM_TO_FRAME(base); i <= (_MEM_TO_FRAME(base + extent - 1)); i++)
		BITVEC_SET(herd, i);
}


/*
 * gets a list of basepointers of all allocated frames.
 *
 * Not very efficient, and just used to get hints for the paging
 * system: O(n^2) in HERD_FRAMES!
 *
 * addrs is a pointer to an array of base pointers that is
 * to be filled. addrcount is the size of that array.
 *
 * returns the amount of allocated frames written to addrs
 *
 */
int get_allocated_frames(int addrcount, void *addrs) {
	int ci, i,fc;
	struct cache_entry ce;

	fc=0;

	for (ci=0; ci < map_cache_entries; ci++) {
		ce = map_cache[ci];

		for(	i = _MEM_TO_FRAME(ce.base); 
		 	i<= (_MEM_TO_FRAME(ce.base + ce.bytes - 1)) 
			&& fc < addrcount; 
			i++) {
				if (BITVEC_GET(herd, i) == 0) {
					*((void **)(addrs)) = _FRAME_TO_MEM(i);
					addrs = addrs + sizeof(void *);
					fc++; 
				}
		}
	}

#ifdef DEBUG
	kprintf("frame %d; %d used\n", i, fc);
#endif

	return fc;
}
