#ifndef __TASK_H
#define __TASK_H

/*
 * Delirium IA32 tasks
 */

#define MAX_THREADS	256


#ifndef ASM

#include <delirium.h>
#include "i386/gdt.h"
#include "i386/paging.h"
#include "dlib/queue.h"



/*
 * running: can be switched to by the STS
 * listening: ignored by the STS. will be set to running when any rants are sent to it.
 * leaving: ignored by the STS. Thread is being killed, but cleanup isn't complete by the thread manager
 * dead: ignored by the STS
 * idle: idle thread. can be switched to by the STS iff no tasks are set to running
 */
typedef enum {
	running, listening, leaving, dead, idle
} thread_state_t;

/*
 * 32 bit TSS: 104 bytes of POWER! (cough)
 *
 * We set up the TSS on kernel entry, and then pretty much ignore it.
 */

#define _pak	__attribute__ ((packed))
typedef struct {
	u_int16_t	previous_task	_pak;
	u_int16_t	reserved0	_pak;
	u_int32_t	esp0		_pak;
	u_int16_t	ss0		_pak;
	u_int16_t	reserved1	_pak;
	u_int32_t	esp1		_pak;
	u_int16_t	ss1		_pak;
	u_int16_t	reserved2	_pak;
	u_int32_t	esp2		_pak;
	u_int16_t	ss2		_pak;
	u_int16_t	reserved3	_pak;
	u_int32_t	pdbr		_pak; // cr3
	u_int32_t	eip		_pak;
	u_int32_t	eflags		_pak;
	u_int32_t	eax		_pak;
	u_int32_t	ecx		_pak;
	u_int32_t	edx		_pak;
	u_int32_t	ebx		_pak;
	u_int32_t	esp		_pak;
	u_int32_t	ebp		_pak;
	u_int32_t	esi		_pak;
	u_int32_t	edi		_pak;
	u_int16_t	es		_pak;
	u_int16_t	reserved4	_pak;
	u_int16_t	cs		_pak;
	u_int16_t	reserved5	_pak;
	u_int16_t	ss		_pak;
	u_int16_t	reserved6	_pak;
	u_int16_t	ds		_pak;
	u_int16_t	reserved7	_pak;
	u_int16_t	fs		_pak;
	u_int16_t	reserved8	_pak;
	u_int16_t	gs		_pak;
	u_int16_t	reserved9	_pak;
	u_int16_t	ldt_ss		_pak;
	u_int16_t	reserveda	_pak;
	u_int16_t	tbit		_pak;
	u_int16_t	iobasemap	_pak;
} tss_t;

typedef u_int32_t	frame_t;


/*
 * Thread data structure.
 *
 * Only contains CPU state and stack state, 
 * not any protection information (see Task.)
 */
typedef struct {
	u_int32_t	edi		_pak;
	u_int32_t	esi		_pak;
	u_int32_t	ebp		_pak;
	u_int32_t	saved_esp	_pak;
	u_int32_t	ebx		_pak;
	u_int32_t	edx		_pak;
	u_int32_t	ecx		_pak;
	u_int32_t	eax		_pak;
	u_int32_t	ss		_pak;
	u_int32_t	ds		_pak;
	void *		eip		_pak;
	u_int32_t	cs		_pak;	// Overloaded: High bit is set when thread is stopped
	u_int32_t	eflags		_pak;
} thread_cpustate_t;

typedef struct {
	thread_state_t	state;

	struct queue	rants;
	size_t		refcount;

	thread_cpustate_t cpu;
	void *		ih_esp;

	size_t		context_switches;
} thread_t;

extern thread_t threads[];
extern size_t global_context_switches;

#endif // ! ASM

#endif // task.h
