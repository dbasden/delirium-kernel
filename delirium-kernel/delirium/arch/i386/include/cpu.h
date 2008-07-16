#ifndef __I386_CPU_H
#define __I386_CPU_H

typedef u_int32_t volatile	Semaphore;

/* We can force a context switch before the timer by calling INT 0x42 */
#define YIELD_CONTEXT	asm volatile (" int $0x42" ::: "memory")
#define KILL_CURRENT_THREAD	asm volatile (" int $0x43" ::: "memory");
#ifndef ENABLE_BOCHS_HACK
#define HALT_CPU	asm(" hlt")
#else
#define HALT_CPU	asm(" nop")
#endif

#define INIT_SEMAPHORE(_s)	_s = 1
#define LOCKED_SEMAPHORE(_s)	((volatile Semaphore)(_s) == 0)

/* MOVL isn't LOCKable, but OR is */
#define RELEASE_SEMAPHORE(_s)	 \
	({asm volatile ("lock or $1, %0" :: "m" (_s) : "memory");})
#define SPIN_WAIT_SEMAPHORE(_s)\
	({asm volatile ("0:\tlock btr $0, %0\n\tjnc 0b" :: "m" (_s) : "memory");})
#define SPIN_YIELD_SEMAPHORE(_s)\
	({asm volatile ("0:\tlock btr $0, %0\n\tjc 1f\n\tint $0x42\n\tjmp 0b\n1:\tnop" :: "m" (_s) : "memory");})

/* _dest = _src if (_dest == expected) */
#define cmpxchg(_dest, _src, _expected) \
	({u_int32_t _result; asm volatile ("\tlock cmpxchg %1,%2" : "=a"(_result) : "r"(_src), "m"(_dest), "0"(_expected) : "memory"  ); _result;})

#define IA32_FLAG_INTERRUPT_ENABLE	0x00000200

#define _INTERRUPTS_ENABLED()	(get_eflags() & IA32_FLAG_INTERRUPT_ENABLE)

void setup_idt();
void addHandler(u_int16_t idt_offset, void *handler);
void enable_interrupts();
void disable_interrupts();
u_int32_t get_eflags();

#endif
