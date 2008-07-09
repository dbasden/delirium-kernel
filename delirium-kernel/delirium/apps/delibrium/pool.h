#ifndef __DELIBRIUM_POOL_H
#define __DELIBRIUM_POOL_H

/*
 * Maintain pools of memory for small allocations.
 *
 * - Only requires a frame or page allocator back-end
 *
 * Allocation sizes are  2**n where  2 <= n <= 10 * (4 -> 1024 bytes)
 *
 * Copyright (c)2006-2008 David Basden <davidb-delirium@rcpt.to>
 * 
 */

/*
 * 2008-07-09	- Modified to put in delibrium
 *
 * 		  The more abstracted backend could be put in here, but
 * 		  delibrium is meant to make life easier if we use it.
 */


/*
 * setup the pool allocator
 */
void setup_pools();

/*
 * allocate from a pool
 * exponent is n where 2 ** n is the amount of memory to be allocated
 */
void * pool_alloc(int exponent);

/*
 * free memory allocated by pool_alloc
 */
void pool_free(int exponent, void *ptr);

#endif
