#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include <delirium.h>

#ifndef ARCH_lucid

#define IDT_INTERRUPT_GATE	0x0e00	//(BIT(9) | BIT(10) | BIT(11))
#define IDT_TRAP_GATE		0x0f00	//(BIT(8) | BIT(9) | BIT(10) | BIT(11))
#define IDT_TASK_GATE		0x0500  // (BIT(8) | BIT(10))

#define IDT_ENTRIES		256

// External interrupts
#define INTR_BASE	32
#define INTR_COUNT	32

// Interrupt aliases
//
#define	INT_TIMER	0
#define INT_KEYBOARD	1

#define IRQ_YIELD	0x42
#define IRQ_KILL	0x43

/* end ifndef ARCH_lucid */
#endif

#ifndef ASM

#define idesc_packed	__attribute__ ((packed))

typedef struct idesc_packed {
        u_int16_t       offset_0_15 idesc_packed;
        u_int16_t       selector idesc_packed;
        u_int16_t       flags_dpl idesc_packed;
	u_int16_t	offset_16_31 idesc_packed;
} interrupt_desc_t;

typedef struct {
	u_int32_t	base; 
	u_int32_t	limit;
} idt_ptr_t;

inline void config_interrupts();

/* inth.S */
void fault_handler();
void trap_handler();
void abort_handler();

void early_timer_isr();
void inth_timer();
void inth_kill_current_thread();

typedef void(*interrupt_handler_t)(void);

/* setup.c */
void add_handler(u_int16_t offset, void (*handler)(void));
void add_c_interrupt_handler(u_int32_t hwirq, void (*handler)(void));

#define add_c_isr	add_c_interrupt_handler


#endif	/* ifndef ASM */

#endif	/* ifndef __INTERRUPTS_H */
