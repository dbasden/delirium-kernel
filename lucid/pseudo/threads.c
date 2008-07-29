/*
 * tasks.c: Hold the context switching logic for delirium
 *
 * TODO: Move the STS out of the context switching code, and into the
 * 	 non-arch dependant code.
 * TODO: Allow use of the timer for something other than calling the STS
 *
 * Copyright (c)2004-2006 David Basden <davidb-delirium@rcpt.to>
 */
#include <delirium.h>
#include <cpu.h>
#include <assert.h>

#include "klib.h"
#include "paging.h"
#include "rant.h"

#include "dlib/queue.h"

#include "i386/task.h"
#include "i386/gdt.h"
#include "i386/cpu.h"
#include "i386/interrupts.h"
#include "i386/pic.h"


#if 0
// Splintering
static Semaphore	INIT_SEMAPHORE(splinter_sem);
static Semaphore	INIT_SEMAPHORE(execute_splinter_sem);   // Only use once above two are set
#endif


extern void do_context_switch();		// cpu.h
extern void kvga_spin(); 			// kvga.c

size_t 		running_thread_id	= 0;
thread_t 	threads[MAX_THREADS];
size_t 		global_context_switches;

/*
 * set the state of the current thread 
 */
void set_thread_state(thread_state_t new_state) {
	threads[running_thread_id].state = new_state;
}

thread_t get_thread_info() {
	return threads[running_thread_id];
}


/*
 * set the current thread to a listening state until there are rants queued for it
 * and then yield context
 *
 * the listening state is advisory for the scheduler, and isn't mandatory, so
 * having a thread inadvertently put into a running state isn't going to break
 * anything (although might be inefficient in some cases). having a thread 
 * inadvertently be put in a listening state when there are rants waiting however 
 * CAN cause deadlock, so we avoid this if at all possible
 */
void await() {
	// We want to avoid deadlock due to a rant being queued after the await() call,
	// but before we change the state to listening.
	//
	// To avoid this case, we set the state to listening, and then check to see if
	// there are any rants queued. If there are, we roll back the state to running.
	// If there aren't, any future rants arriving will wake the thread
	// (assuming sending a rant will first 
	
	volatile thread_state_t *thread_state_p = &(threads[running_thread_id].state);
	if (! (*thread_state_p == running || *thread_state_p == listening))
		return;

	*thread_state_p = listening;

	if (! QUEUE_ISEMPTY(&(threads[running_thread_id].rants))) 
		*thread_state_p = running;
	else
		yield();
}

/*
 * set a listening or running thread to a running state
 */
void wake_thread(size_t thread_id) {
	thread_state_t *thread_state_p = &(threads[thread_id].state);
	if (*thread_state_p == running || *thread_state_p == listening)
		*thread_state_p = running;
}

