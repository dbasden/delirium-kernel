/*
 * Facade for ASM routines etc
 */

#include <delirium.h>

#include "klib.h"
#include "kvga.h"
#include "ktimer.h"

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

	kmemcpy(idtEntry, (void *)(&_idt_interrupt_template), sizeof(interrupt_desc_t));
	idt[offset].offset_0_15 = hptr.w[0];
	idt[offset].offset_16_31 = hptr.w[1];
#ifdef DEBUG
	dump_idt_entry(offset);
#endif
}

volatile interrupt_handler_t shimmed_hwirq_handler_hooks[16];

/* 
 * handle a shimmed IRQ 
 */
void handle_shimmed_isr(u_int32_t hwirq) {
	if (shimmed_hwirq_handler_hooks[hwirq] != NULL) {
		(shimmed_hwirq_handler_hooks[hwirq])();
		return;
	}
	kprintf("(hwirq %d)", hwirq);
}



/* wrap a C hardware IRQ  handler and install it in the IDT
 * the interrupt is then unmasked.
 *
 * for userspace calls - calls add_c_isr 
 * (which is a macro that needs our symbol table)
 *
 * Users can just write their own ASM interrupt handlers if they really
 * want, but it's probably more stable to have all the ISR bugs in one place.
 */
void add_c_interrupt_handler(u_int32_t hwirq, void (*handler)(void)) {
	kprintf("%s: %d -> 0x%8x\n", __func__, hwirq, handler);
	shimmed_hwirq_handler_hooks[hwirq] = handler;
	pic_unmask_interrupt(hwirq);
}

/* 
 * mask the interrupt and replace the interrupt handler 
 * with the default one
 */
void remove_interrupt_handler(u_int32_t hw_interrupt) {
	pic_mask_interrupt(hw_interrupt);
	shimmed_hwirq_handler_hooks[hw_interrupt] = NULL;
}

void setup_memory() {
	kdebug("setup_memory entry");
	kdebug("calling install_gdt");
	install_gdt();
	kdebug("setup_memory exit");
}

// set the int 0 timer to trigger a bit more than every ~53ms 
void setup_timer() {

	// 1.193182 MHz input clock
	// divide by 1193 (0x04a9) to get around 1000Hz (~1000.15256 Hz)
	kdebug("setting int0 timer ");	
	pic_mask_interrupt(INT_TIMER);
	outb(0x43, 0x34); // 00110100b - chan 0, freq. divider, set lo/hi byte
	outb(0x40, 0xa9); // Low byte of divider to chan 0 reload value
	outb(0x40, 0x04); // High byte of divider to chan 0 reload value


	kdebug("initialising ktimers");

	/* Say we call it every 1000 useconds, when we really do every 999.847 useconds) */
	ktimer_init(1000); 
	
	add_handler(INTR_BASE + INT_TIMER, &early_timer_isr);
	pic_unmask_interrupt(INT_TIMER);
}

void setup_interrupts() {
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
	int i;
	for (i=0; i<16; ++i) 
		shimmed_hwirq_handler_hooks[i] = 0;
	kdebug("Setting up default handler for external interrupts");
	extern void isr_shim_0(); extern void isr_shim_1(); extern void isr_shim_2();
	extern void isr_shim_3(); extern void isr_shim_4(); extern void isr_shim_5();
	extern void isr_shim_6(); extern void isr_shim_7(); extern void isr_shim_8();
	extern void isr_shim_9(); extern void isr_shim_10(); extern void isr_shim_11();
	extern void isr_shim_12(); extern void isr_shim_13(); extern void isr_shim_14();
	extern void isr_shim_15();
	add_handler(INTR_BASE + 0,  &isr_shim_0);
	add_handler(INTR_BASE + 1,  &isr_shim_1);
	add_handler(INTR_BASE + 2,  &isr_shim_2);
	add_handler(INTR_BASE + 3,  &isr_shim_3);
	add_handler(INTR_BASE + 4,  &isr_shim_4);
	add_handler(INTR_BASE + 5,  &isr_shim_5);
	add_handler(INTR_BASE + 6,  &isr_shim_6);
	add_handler(INTR_BASE + 7,  &isr_shim_7);
	add_handler(INTR_BASE + 8,  &isr_shim_8);
	add_handler(INTR_BASE + 9,  &isr_shim_9);
	add_handler(INTR_BASE + 10, &isr_shim_10);
	add_handler(INTR_BASE + 11, &isr_shim_11);
	add_handler(INTR_BASE + 12, &isr_shim_12);
	add_handler(INTR_BASE + 13, &isr_shim_13);
	add_handler(INTR_BASE + 14, &isr_shim_14);
	add_handler(INTR_BASE + 15, &isr_shim_15);

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

