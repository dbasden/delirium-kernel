/*
 * Maintain pools of memory for small allocations.
 *
 * - Only requires a frame or page allocator back-end
 *
 * Allocation sizes are  2**n where  2 <= n <= 10 * (4 -> 1024 bytes)
 *
 * Copyright (c)2006 David Basden <davidb-delirium@rcpt.to>
 */

/*
 * Delirium pre: paging must be running
 */

#include <delirium.h>
#include <cpu.h>
#include <assert.h>
#include <paging.h>

#include "dlib/memherd.h"

#define HUNK_SIZE	PAGE_SIZE
#define MAX_POWER	10

#define getHunk kgetpage
#define releaseHunk kfreepage

#define warnx(_x)


/*
 * Chained index
 */
struct mem_pool {
	struct pool_index *next;
	int alloc_size;			/* Size of the blocks allocated in this pool. 
					   0 if unused pool. */

	herd_t herd; 			/* Herd allocator for the pool */
	void *pool;			/* Pool itself */
};

struct master_index {
	struct master_index *next;	/* Link to the next index hunk */
	struct master_index *prev;	/* Link to the previous index hunk */
	size_t pool_count;		/* Pools in this index hunk */
	size_t used_pools;

	/* Master mutex. Lock on any operation until have pool mutex. 
	 * Only relevant for the first master index */

	/* Index for pools that have free memory. Index is n where 2**n is hunk 
	 * size.
	 *
	 * e.g. pool_index[4] gives an index to a pool for allocation size 2**4 
	 * 	or 16 bytes.
	 *
	 * Only valid in the first master index, not any chained ones. Value is
	 * NULL iff there are currently no pools with free memory for that allocation
	 * size. 
	 */
	struct mem_pool * pool_index[MAX_POWER+1];		

	/* Pool index records in this index hunk */
	struct mem_pool pools[];
};

static void allocate_pool(struct mem_pool *p, int alloc_size) {
	assert(p->alloc_size == 0);

	p->pool = getHunk();
	p->alloc_size = alloc_size;
	p->herd = new_memherd(p->pool, HUNK_SIZE, alloc_size);
}

static void release_pool(struct mem_pool *p) {
	assert(p->alloc_size != 0);

	p->alloc_size = 0;
	releaseHunk(p->pool);
}

static int is_within_pool(struct mem_pool *pool, void *ptr) {
	return (ptr >= pool->pool && ptr < (pool->pool + HUNK_SIZE));
}

/* Allocate and init a new master index. */
static struct master_index * create_master_index(struct master_index *parent) {
	struct master_index *mi;
	int i;

	mi = getHunk();
	mi->prev = NULL;
	mi->next = NULL;

	mi->pool_count = (HUNK_SIZE - (((void *)(mi->pools)) - ((void *)mi)))\
			 	/ sizeof(struct mem_pool);
	mi->used_pools = 0;

	for (i=0; i<MAX_POWER+1; i++) mi->pool_index[i] = NULL;
	for (i=0; i<mi->pool_count; i++) {
		mi->pools[i].alloc_size = 0;
	}

	if (parent != NULL) {
		mi->next = parent->next;
		mi->prev = parent;
		parent->next = mi;
		if (mi->next != NULL) { mi->next->prev = mi; }
	}

	return mi;
}

#if 0

/*****
 * Currently not used, although code is tested and working.
 *
 * Will be needed in future - dgb
 */
static void release_master_index(struct master_index *mi) {
	int i;

	if (mi->prev != NULL) { mi->prev->next = mi->next; }
	if (mi->next != NULL) { mi->next->prev = mi->prev; }

	for (i=0; i<mi->pool_count; i++) {
		if (mi->pools[i].alloc_size != 0) {
			warnx("Releasing master index with allocated pools");
			release_pool(&(mi->pools[i]));
		}
	}
	releaseHunk(mi);
}
#endif

/*
 * Find a slot to allocate a new pool into.  Allocate a new master index if needed
 */
static struct mem_pool * get_free_pool_slot(struct master_index *master) {
	int i;

	while ((master->used_pools != master->pool_count) && (master->next != NULL)) 
		master = master->next;
	
	if (master->used_pools == master->pool_count)
		master = create_master_index(master);

	for (i=0; i<master->pool_count; i++)
		if (master->pools[i].alloc_size == 0)
			return &(master->pools[i]);

	assert(0);
	return NULL; /* Compiler happyness */
}

static struct mem_pool * find_owner(struct master_index *master, int exponent, void *ptr) {
	size_t size;

	size = 1<<exponent;

	for (;master != NULL; master=master->next) {
		int i;

		for (i=0; i<master->pool_count; i++) {
			if ((master->pools[i].alloc_size == size) && \
			    is_within_pool( &(master->pools[i]), ptr)) {
				return &(master->pools[i]);
			}
		}
	}

	return NULL;
}

/*
 * Find a pool of size n**exponent with at least 1 item free in the pool
 */
static struct mem_pool * get_free_pool(struct master_index *master, int exponent) {
	struct mem_pool *p;
	assert(exponent <= MAX_POWER);
	
	p = (master->pool_index)[exponent];

	if (p == NULL) {
		kprintf("[kernel/pool.c get_free_pool: master %x : pools for exp %d are full]\n", master, exponent);
		/* All pools of that size are full. Make another. */
		p = get_free_pool_slot(master);
		allocate_pool(p, 1 << exponent);
		master->pool_index[exponent] = p;
	} 
	return p;
}

void update_available_cache(struct master_index *m, int exponent) {
	struct master_index *mp;
	int i;
	size_t size;

	size = 1 << exponent;

	if ((m->pool_index[exponent] != NULL) &&  \
		m->pool_index[exponent]->alloc_size == size &&  \
		m->pool_index[exponent]->herd.free_blocks > 0)
		return; /* No need to update cache */


	for (mp = m; mp != NULL; mp = mp->next) {
		for (i=0; i<mp->pool_count; i++) {
			if ((mp->pools[i].alloc_size == size) && \
			    mp->pools[i].herd.free_blocks > 0)  {
				m->pool_index[exponent] = &(mp->pools[i]);
				return;
			}
		}
	}

	m->pool_index[exponent] = NULL;
}


/* Allocate 2**exponent bytes from the pool where exponent <= 10 */
static void * palloc(struct master_index *master, int exponent) {
	struct mem_pool *p;
	void *mem;

	if (exponent < 2 || exponent > MAX_POWER) return NULL;

	p = get_free_pool(master, exponent);
	mem = memherd_getBlock(&(p->herd));
	
	if (master->pool_index[exponent]->herd.free_blocks <= 0) {

		/* Invalidate 'has available' cache */
		master->pool_index[exponent] = NULL;

		/* search for another */
		update_available_cache(master, exponent);
	}

	return mem;
}

static void pfree(struct master_index *master, int exponent, void *ptr) {
	struct mem_pool *pool;

	pool = find_owner(master, exponent, ptr);
	if (pool == NULL) {
		warnx("Asked to free a pointer that wasn't ours");
		return;
	}

	memherd_freeBlock(&(pool->herd), ptr);

	/* TODO: Fix the below code, which doesn't actually work :-( 
	 * It doesn't remove the pool from the master pools list
	 */
#if 0
	if (pool->herd.total_blocks == pool->herd.free_blocks) {
		/* Pool is empty. Release it  */
		release_pool(pool);
		if (master->pool_index[exponent] == pool) {
			master->pool_index[exponent] = NULL;
			update_available_cache(master, exponent);
		}

		/* TODO: Release chained master index iff empty */
	}
#endif
}


/*****************/

static Semaphore pool_mutex;
static volatile struct master_index *kmaster_index;

void setup_pools() {
	INIT_SEMAPHORE(pool_mutex);
	SPIN_WAIT_SEMAPHORE(pool_mutex);
	kmaster_index = create_master_index(NULL);
	RELEASE_SEMAPHORE(pool_mutex);
}

void * kpool_alloc(int exponent) {
	void *ptr;
	SPIN_WAIT_SEMAPHORE(pool_mutex); {
		ptr = palloc(kmaster_index, exponent);
	} RELEASE_SEMAPHORE(pool_mutex);

	return ptr;
}

void kpool_free(int exponent, void *ptr) {
	SPIN_WAIT_SEMAPHORE(pool_mutex); {
		pfree(kmaster_index, exponent, ptr);
	} RELEASE_SEMAPHORE(pool_mutex);
}
