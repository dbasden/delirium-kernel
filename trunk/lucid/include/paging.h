#ifndef _PAGING_H
#define _PAGING_H

#ifdef ARCH_i386
#include "i386/paging.h"
#endif

/* MUST be the same as FRAME_SIZE */
#define PAGE_SIZE	4096

extern void setup_paging();
extern void paging_add_to_dir(page_table_t dir, void *logical, void *physical);
extern void paging_remove_from_dir(page_table_t dir, void *logical);
  
extern void add_to_pagedir(page_table_t dir, void *logical, void *physical);
extern void remove_from_pagedir(page_table_t dir, void *logical);

extern void *kgetpage();
extern int kgetpages(int, void **);
extern void kfreepage(void *);
extern void kfreepages(int, void **);

extern void *kherdpages(int, void **);

#endif // _PAGING_H
