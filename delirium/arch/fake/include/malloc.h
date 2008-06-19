#ifndef __FAKE_MALLOC_H
#define __FAKE_MALLOC_H

/* Allocate SIZE bytes of memory.  */

#undef malloc
extern void *malloc (size_t __size);

#endif
