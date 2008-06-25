#include <delirium.h>
#include <multitask.h>
#include "delibrium.h"
#include "paging.h"
#include "rant.h"

/*
 * Default thread behaviour if not doing anything: process rant queue
 */
void thread_ret() {
	while (get_thread_info().refcount) {
		believe();
		yield();
	}

#ifdef ARCH_i386 
	asm volatile ("	int $0x43");
#endif
	for (;;);
	
}

/*
 * start a new thread, allocating a single pageframe as a stack
 */
void new_thread(void *threadEntry) {
	void * stack;

	// Take a single frame for the stack
	//
	stack = getpage() + PAGE_SIZE;

	// push the address of thread_ret on the stack; if threadEntry exits,
	// and the stack wasn't munged, it will call that on exit.
	//
	stack -= sizeof(void *);
	*((void **)stack) = thread_ret;
	splinter(threadEntry, stack);
}
