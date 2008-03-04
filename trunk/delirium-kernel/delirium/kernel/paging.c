#include <delirium.h>

#include "frameherd.h"
#include "paging.h"
#include "klib.h"

/* 2 gig worth of address space
 *
 * Let's not run out
 */
void * orderBase = (void *) 0x80000000;

/*
 * get a frame from the frameherd, and place it into the page directory
 */
void * kgetpage() {
	void *frame;

	if (take_from_herd(1, &frame) != 1) {
		kprint("kgetpage(): failed to take from herd");
		kpanic();
	}

	paging_add_to_dir(kernel_page_dir, frame, frame);
	return frame;
}

int kgetpages(int wanted, void **pages) {
	int got,i;

	got = take_from_herd(wanted, pages);
	for (i=0; i<got; i++)
		paging_add_to_dir(kernel_page_dir, pages[i], pages[i]);

	return got;
}

/*
 * herd pages into an order. return a pointer to that ordering
 * or NULL if the order doesn't work
 *
 * pages are like goats. they don't enjoy being herded.
 */
void * kherdpages(int pagecount, void **pages) {
	int i;

	if ((pagecount + ((size_t)orderBase / PAGE_SIZE)) > (0xffffffff / PAGE_SIZE))
		return NULL;

	for (i = 0; i < pagecount; i++) {
		paging_add_to_dir(kernel_page_dir, orderBase, pages[i]);
		orderBase += PAGE_SIZE;
	}
	return orderBase - (PAGE_SIZE * pagecount);
}

void kfreepage(void *page) {
	paging_remove_from_dir(kernel_page_dir, page);
	return_to_herd(1, &page);
}

void kfreepages(int pagecount, void **pages) {
	for (pagecount--; pagecount<=0; pagecount--)
		kfreepage(pages[pagecount]);
}
