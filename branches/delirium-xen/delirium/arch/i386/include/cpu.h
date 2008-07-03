#ifndef __I386_CPU_H
#define __I386_CPU_H

typedef volatile u_int32_t	Semaphore;

/* We can force a context switch before the timer by calling INT 0x42 */
#define YIELD_CONTEXT	asm(" int $0x42")
#define KILL_CURRENT_THREAD	asm(" int $0x43");
#define HALT_CPU	asm(" hlt")

#define INIT_SEMAPHORE(_s)	_s = 1
#define LOCKED_SEMAPHORE(_s)	((_s) <= 0)
#define RELEASE_SEMAPHORE(_s)	( _s++ )

#define SPIN_WAIT_SEMAPHORE(_s)\
	({asm volatile ("0:\tlock btr $0, "#_s"\n\tjnc 0b");})

void setup_idt();
void addHandler(u_int16_t idt_offset, void *handler);
void enable_interrupts();
void disable_interrupts();

#endif
