/*
 * Facade for ASM routines etc
 */

#include <delirium.h>

#include "klib.h"
#include "kvga.h"

#include "i386/cpu.h"
#include "i386/mem.h"
#include "i386/interrupts.h"
#include "i386/pic.h"
#include "i386/io.h"


extern interrupt_desc_t _idt_base;
extern interrupt_desc_t _idt_interrupt_template;
static interrupt_desc_t * idt = &_idt_base;

union uptr { void * vp; u_int16_t w[2];};

#ifdef DEBUG
static void dump_idt_entry(int desc) {
	
	kprintf("IDT[%d] Offset 0x%4x%4x, GDT Select %x, Flags/DPL %b\n",
			desc,
			idt[desc].offset_16_31, idt[desc].offset_0_15,
			idt[desc].selector, idt[desc].flags_dpl);
}
#endif

void add_handler(u_int16_t offset, void (*handler)(void)) {
	union uptr hptr = { .vp = handler };
	void *idtEntry = &(idt[offset]);

	kmemcpy(idtEntry, (void *)(&_idt_interrupt_template), 16);
	idt[offset].offset_0_15 = hptr.w[0];
	idt[offset].offset_16_31 = hptr.w[1];
#ifdef DEBUG
	dump_idt_entry(offset);
#endif
}


void setup_memory() {
	kdebug("setup_memory entry");
	kdebug("calling install_gdt");
	install_gdt();
	kdebug("setup_memory exit");
}

// set the int 0 timer to trigger a bit more than every ~53ms 
// pre: interrupts are disabled
void setup_timer() {
	// 1.193182 MHz input clock
	// divide by 1193 (0x04a9) to get around 1000Hz
	kdebug("setting int0 timer ");	
	outb(0x43, 0x34); // 00110100b - chan 0, freq. divider, set lo/hi byte
	outb(0x40, 0xa9); // Low byte of divider to chan 0 reload value
	outb(0x40, 0x04); // High byte of divider to chan 0 reload value
}

void setup_interrupts() {
	int i;

	kdebug("setup_interrupts entry");
	kdebug("disable_interrupts");
	asm volatile ("	CLI");

	kdebug("calling configure_pics");
	configure_pics();

	kdebug("Clearing all of the handlers");
	kmemset((void *)idt, 0, 256 * 8);

	/*
	 * faults - IP on executing instruction
	 * 
	 * faults without error on stack : 0, 5, 6, 7, 19, 16
	 * faults with error on stack: 10, 11, 12, 13, 14, 17a
	 */
	kdebug("Adding handler for faults");
	add_handler(0, &fault_handler); add_handler(5, &fault_handler);
	add_handler(6, &fault_handler); add_handler(7, &fault_handler);
	add_handler(19, &fault_handler); add_handler(16, &fault_handler);

	add_handler(10, &fault_handler); add_handler(11, &fault_handler);
	add_handler(12, &fault_handler); add_handler(13, &fault_handler);
	add_handler(14, &fault_handler); add_handler(17, &fault_handler);
	
	/*
	 * traps - IP after executing instruction
	 *
	 * traps: 2, 3, 4
	 */
	kdebug("Adding handler for traps");
	add_handler(2, &trap_handler); add_handler(3, &trap_handler);
	add_handler(4, &trap_handler);

	/*
	 * aborts (with error): 8, 18
	 */
	kdebug("Adding handler for aborts");
	add_handler(8, &abort_handler); add_handler(18, &abort_handler);

	/*
	 * default interrupt handler for external interrupts
	 */
	kdebug("Setting up default handler for external interrupts");
	for (i=INTR_BASE; i<INTR_BASE + INTR_COUNT; i++)
		add_handler(i, &default_interrupt_handler);

	kdebug("calling setup_idt");
	setup_idt();
	kdebug("enabling interrupts");
	asm volatile ("	STI");
}

#if 0
_IVEC_0	"Divide by zero FAULT"
_IVEC_1	"RESERVED IVEC 1"
_IVEC_2	"NMI TRAP"
_IVEC_3	"Breakpoint (INT 3) TRAP"
_IVEC_4	"Overflow (INTO) TRAP"
_IVEC_5	"BOUND range exceeded Fault"
_IVEC_6	"Invalid opcode or UD2 FAULT"
_IVEC_7	"No maths copro FAULT"
_IVEC_8	"Double fault ABORT (error = 0)"
_IVEC_9	"Copro seg overrun FAULT"
_IVEC_10	"Invalid TSS FAULT (err)"
_IVEC_11	"Segment not present FAULT (err)"
_IVEC_12	"Stack-Segment FAULT (err)"
_IVEC_13	"General Protection FAULT (err)"
_IVEC_14	"Page Fault (err)"
_IVEC_15	"RESERVED IVEC 15"
_IVEC_16	"x87 FPU FAULT"
_IVEC_17a	"Alignment check FAULT err=0"
_IVEC_18	"Machine check ABORT maybe err"
_IVEC_19	"SIMD FPE FAULT"
/* 20-31: Intel reserved */
/* 32-255: User defined interrupts */
#endif

