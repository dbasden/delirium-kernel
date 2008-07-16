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


// Splintering
static Semaphore	INIT_SEMAPHORE(splinter_sem);
		static void *		sem_locked_splinter_to;	// Only use when holding semaphore
		static void *		sem_locked_splinter_sp;	// Only use when holding semaphore
static Semaphore	INIT_SEMAPHORE(execute_splinter_sem);   // Only use once above two are set


extern void do_context_switch();		// cpu.h
extern void kvga_spin(); 			// kvga.c

size_t 		running_thread_id;
thread_t 	threads[MAX_THREADS];
size_t 		global_context_switches;

/*
 * set the state of the current thread 
 */
void set_thread_state(thread_state_t new_state) {
	threads[running_thread_id].state = new_state;
}

/*
 * contains the short term scheduler logic
 */
static inline u_int32_t get_task_to_run() {
	int id;
	
	// Very simple round-robin
	id = (running_thread_id+1) % MAX_THREADS;
	while (id != running_thread_id  &&  (threads[id].state != running))
		id = (id+1) % MAX_THREADS;

	// Look for an idle thread
	if (id == running_thread_id && threads[id].state != running) {
		id = (running_thread_id+1) % MAX_THREADS;
		while (id != running_thread_id  &&  (threads[id].state != idle))
			id = (id+1) % MAX_THREADS;
	}
	else kvga_spin();

	return id;
}

/* called only from within a context switch 
 * returns the ID of the new thread
 */
static inline size_t copy_current_thread(size_t outgoing_esp) {
	size_t new_thread_id;	
	
	// Find a free thread id
	new_thread_id = (running_thread_id+1) % MAX_THREADS;
	while (new_thread_id != running_thread_id &&
			(threads[new_thread_id].state != dead))
		new_thread_id = (new_thread_id+1) % MAX_THREADS;
	Assert(running_thread_id != new_thread_id);

	kmemcpy( (void *) &(threads[new_thread_id].cpu), 
		 (void *) outgoing_esp,
		 sizeof(thread_cpustate_t));

	threads[new_thread_id].cpu.eip = sem_locked_splinter_to;
	threads[new_thread_id].ih_esp = sem_locked_splinter_sp;

	// Copy the execution context to be restored onto the new stack
	//
	threads[new_thread_id].ih_esp -= sizeof(thread_cpustate_t);
	kmemcpy(threads[new_thread_id].ih_esp, 
		(void *) &(threads[new_thread_id].cpu) ,
		sizeof(thread_cpustate_t));

	// Setup other thread member variables
	threads[new_thread_id].state = running;
	threads[new_thread_id].refcount = 0;
	threads[new_thread_id].context_switches = 0;
	QUEUE_INIT(&(threads[new_thread_id].rants));

	return new_thread_id;
}

/*
 * called from the lower-level context switch handler with CLI. Takes the
 * outgoing ESP, and returns the ESP for the task to be switched to
 */
u_int32_t get_next_switch(u_int32_t outgoing_esp) {

	/* save outgoing thread stuff */
	kmemcpy(	(void *) &(threads[running_thread_id].cpu), 
			(void *) outgoing_esp,
			sizeof(thread_cpustate_t));
	threads[running_thread_id].ih_esp = (void *) outgoing_esp;

	if (LOCKED_SEMAPHORE(execute_splinter_sem)) {
		/*
		 * Create a new thread from current execution context
		 * (splinter) and change context to that thread
		 */
		running_thread_id  = copy_current_thread(outgoing_esp);
		RELEASE_SEMAPHORE(execute_splinter_sem);  // Release semaphore once done
	} else {
		running_thread_id = get_task_to_run();
	}
	++global_context_switches;
	++(threads[running_thread_id].context_switches);

	Assert(running_thread_id < MAX_THREADS);

	// Return ESP containing enough to restore context of new running_thread_id
	return (u_int32_t) threads[running_thread_id].ih_esp;
}

u_int32_t kill_current_thread(u_int32_t outgoing_esp) {
	size_t	brick_wall	= running_thread_id;
	u_int32_t new_esp;

	threads[brick_wall].state =leaving;

	/* The outgoing esp isn't very useful to us at
	 * this point. It's quite possible that it's
	 * the emergency stack. Here we will do an evil hack
	 * and assume that the stack at the last context
	 * switch is the one we want to free. Ick.
	 * EVIL HACK. Delete the same time as the one below
	 */
	outgoing_esp = (u_int32_t) threads[running_thread_id].ih_esp;

	new_esp = get_next_switch(outgoing_esp);

	if (running_thread_id == brick_wall) {
		kprint("Some idiot killed the last kernel thread, and didn't switch off the lights!");
		kpanic();
	}
	
	/*
	 * TODO: Remove this EVIL HACK! It asssumes that the stack page is meant to
	 * 	 be freed here, which may be the case for kthreads, but not enforced
	 * 	 by the ABI
	 */
	kfreepage((void *) threads[brick_wall].ih_esp);
	threads[brick_wall].state = dead;

	return new_esp;
}

/*********************************/

static void fill_tss(tss_t *tss, void *start) {

	kmemset((void *)tss, '\0', sizeof(tss));
	tss->eip = (u_int32_t) start;
	tss->cs = 0x08;
	tss->ds = 0x10; tss->ss = 0x10; tss->es = 0x10; tss->fs = 0x10;
	tss->gs = 0x10; tss->ss0 = 0x10; tss->ss1 = 0x10; tss->ss2 = 0x10;
}

static void fill_tssdesc(gdt_entry_t *td, u_int32_t base, u_int32_t limit) {

	td->base_low = (u_int16_t)(base & 0xffff);	
	td->base_16_23 = (u_int8_t)((base & 0x00ff0000) >> 16);
	td->base_24_31 = (u_int8_t)((base & 0xff000000) >> 24);
	td->limit_low = (u_int16_t)(limit & 0xffff);
	td->access_flags = GDT_TYPE_TSS_AVAIL;
	td->flags_limit_hi = (u_int8_t) ((limit & 0x000f0000) >> 16) ;
}

extern tss_t init_tss;
extern void install_init_tr();
extern void save_first_tss();

void setup_tasks() {
	int i;

	/* Setup the TSS  - We only have once TSS entry for the kernel */
	fill_tss(&init_tss, kpanic);
	save_first_tss();

	fill_tssdesc(
		( ((void *)(gdt_descriptor.gdt)) + GDT_KERNEL_TASKS),
		(u_int32_t) &init_tss,
		sizeof(tss_t)
	);

	/* Global context switch counter */
	global_context_switches = 0;

	/* Initial thread is the current execution context; ID 0 */
	running_thread_id = 0;
	threads[0].state = running;
	threads[0].refcount = 0;
	QUEUE_INIT(&(threads[0].rants));

	/* Need to setup the task register, otherwise intel gets all sad */
	install_init_tr();

	/*
	 * Flag all threads apart from the first as non-running
	 */
	for (i=1; i<MAX_THREADS;i++)
		threads[i].state = dead;

	/* Handler for context switching on demand through INT 0x42*/
	add_handler(IRQ_YIELD, &do_context_switch);
	add_handler(IRQ_KILL, &inth_kill_current_thread);
	
	/* Hook the timer interrupt to do our switches for us */
	add_handler(INTR_BASE + INT_TIMER, &inth_timer);
	pic_unmask_interrupt(0);
}


void splinter(void *taskentry, void *newsp) {
	/*
	 * TODO: Fix logic error.
	 *
	 * If we hold splinter_sem, the next time the STS is called in
	 * a timer interrupt it will assume that we want to split stuff off
	 * using the data in sem_locked_spliter_to and sem_locked_splinter_sp
	 *
	 * The real problem is if we get a timer interrupt BEFORE we set these
	 * two variables. We really should splinter stuff off without relying
	 * on the STS to do it; Putting the new entry and stack pointer into the
	 * process queue, and just waiting for the STS to deal should be enough.
	 * We can then duplicate the previous behaviour by yielding context once
	 * all this is setup.
	 */

	SPIN_WAIT_SEMAPHORE(splinter_sem); // Get lock on splintering

	sem_locked_splinter_to = taskentry;
	sem_locked_splinter_sp = newsp;

	SPIN_WAIT_SEMAPHORE(execute_splinter_sem); // Signal STS that above two vars are set

			/*
			 * context switch handler will call short term scheduler
			 * (get_next_switch), where there are hooks for the rest
			 * of the splinter.
			 *
			 * Below we explicitly invoke the STS to make sure this
			 * will happen, but we may have already hit the STS from a
			 * timer interrupt or other interrupt
			 */
	YIELD_CONTEXT;

	// We DONT release execute_splinter_sem semaphore here. It will be released when our 
	// splinter request is handled! We do however release splinter_sem
	RELEASE_SEMAPHORE(splinter_sem); 
	
}

/*
 * Default kernel thread behaviour if not doing anything:
 * process rant queue
 */
void kthread_ret() {
	while (threads[running_thread_id].refcount) {
		if (QUEUE_ISEMPTY(&(threads[running_thread_id].rants))) {
			yield();
		} else {
			believe();
		}
	}

	/* Threads should really call exit_kthread */
	kprintf("<Bad thread exit: thread id %u>", running_thread_id);
	KILL_CURRENT_THREAD;
}

#define memcpy	kmemcpy
thread_t get_thread_info() {
	return threads[running_thread_id];
}

/*
 * start a new kernel thread, allocating a single pageframe as a stack
 */
void new_kthread(void *threadEntry) {
	void * stack;

	// Take a single frame for the stack
	//
	stack = kgetpage() + PAGE_SIZE;

	// push the address of kthread_ret on the stack; if threadEntry exits, 
	// and the stack wasn't munged, it will call that on exit.
	//
	stack -= sizeof(void *);
	*((void **)stack) = kthread_ret;
	splinter(threadEntry, stack);
}


/*
 * Kill off the current thread
 */
void exit_kthread() {
	KILL_CURRENT_THREAD;
}

void yield() {
	YIELD_CONTEXT;
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
		YIELD_CONTEXT;
}

/*
 * set a listening or running thread to a running state
 */
void wake_thread(size_t thread_id) {
	thread_state_t *thread_state_p = &(threads[thread_id].state);
	if (*thread_state_p == running || *thread_state_p == listening)
		*thread_state_p = running;
}

