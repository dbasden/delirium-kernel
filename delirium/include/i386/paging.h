#ifndef _I386_PAGING_H
#define _I386_PAGING_H

/*
 * Paging support under IA32 for DeLiRiUM
 *
 * (c)2005 David Basden <davidb-delirium@rcpt.to>
 */

#define PT_PRESENT		(1)
#define PT_RW			(1 << 1)
#define PT_USER_PRIV		(1 << 2)
#define PT_WRITE_THROUGH	(1 << 3)
#define PT_CACHE_DISABLE	(1 << 4)
#define PT_ACCESSED		(1 << 5)
#define PT_DIRTY		(1 << 6)
#define PT_ATTR_IDX		(1 << 7)
#define PT_GLOBAL		(1 << 8)

/* We can use bits 9, 10 and 11 in both PT and PD */

/* MUST be the same as the frame size */
#define PAGE_SIZE	4096

#ifndef _ASM__
typedef	u_int32_t * page_table_t;

extern void set_pdbr(void *);
extern void disable_paging();
extern void enable_paging();
extern void handle_pf();

extern page_table_t kernel_page_dir;
#endif

#endif // _I386_PAGING_H
