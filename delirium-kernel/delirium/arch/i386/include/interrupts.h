#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include <delirium.h>

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
void default_interrupt_handler();
void fault_handler();
void trap_handler();
void abort_handler();

void early_timer_isr();
void inth_timer();
void inth_kill_current_thread();

typedef void(*interrupt_handler_t)(void);

/* setup.c */
void add_handler(u_int16_t offset, void (*handler)(void));

/* black magic */

#if 0
#define __add_c_isr(__hwint, __handler)\
	        ({extern void isr_hook_ ## __hwint();\
	          (&C_isr_p)[(__hwint)] = (void *)(__handler);\
	          add_handler(INTR_BASE+(__hwint), &isr_hook_ ## __hwint);})
#define add_c_isr(__hwint, __handler) __add_c_isr(__hwint, __handler)
#endif

#define __add_c_isr(__hwint, __handler)\
                ({ (&C_isr_p)[(__hwint)] = (void *)(__handler);\
                  add_handler(INTR_BASE+(__hwint), c_isr_wrapper_table[(__hwint)]);})
#define add_c_isr(__hwint, __handler) __add_c_isr(__hwint, __handler)

extern	void **C_isr_p;
extern interrupt_handler_t c_isr_wrapper_table[];


#endif	/* ifndef ASM */

#endif	/* ifndef __INTERRUPTS_H */
