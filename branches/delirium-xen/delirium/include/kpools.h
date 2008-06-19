#ifndef _KPOOLS_H
#define _KPOOLS_H

/*
 * Maintain pools of memory for small allocations.
 *
 * - Only requires a frame or page allocator back-end
 *
 * Allocation sizes are  2**n where  2 <= n <= 10 * (4 -> 1024 bytes)
 *
 * Copyright (c)2006 David Basden <davidb-delirium@rcpt.to>
 */

void setup_pools();
void * kpool_alloc(int exponent);
void kpool_free(int exponent, void *ptr);

#endif
