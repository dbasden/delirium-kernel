#ifndef __CPU_H
#define __CPU_H

#ifdef ARCH_i386
#include "i386/cpu.h"
#else
#warn "Semaphores aren't being used -- write a semaphore implementation for this arch!"
#define INIT_SEMAPHORE
#define SPIN_WAIT_SEMAPHORE
#define RELEASE_SEMAPHORE
#endif

extern size_t          running_thread_id;

/* Setup */
void setup_tasks();
void setup_paging();


/* Halt the current CPU */
void khalt();

/* Start a new kernel thread */
void new_kthread(void *);

/* Yield your execution context, and make the CPU your willing bitch */
void yield();

/* Stop execution of the current thread and delete it*/
void exit_kthread();

#endif
