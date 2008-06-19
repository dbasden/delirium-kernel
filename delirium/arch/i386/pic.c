/*
 * PIC handling
 */

#include <delirium.h>
#include <klib.h>
#include <kvga.h>
#include <i386/cpu.h>
#include <i386/mem.h>
#include <i386/interrupts.h>
#include <i386/io.h>

inline void pic_mask_all_interrupts() {
	outb(0x21, 0xff);
	outb(0xA1, 0xff);
}

inline void pic_unmask_interrupt(char intr) {
	if (intr < 0x8)  outb(0x21, inb(0x21) & ~( 1 << intr ) );
	else  outb(0xA1, (inb(0xA1) & ~(  1 << (intr-8) )));
}
inline void pic_mask_interrupt(char intr) {
	if (intr < 0x8)  outb(0x21, inb(0x21) | ( 1 << intr ) );
	else  outb(0xA1, (inb(0xA1) | (  1 << (intr-8) )));
}

void configure_pics() {
	/* 
	 * Command:	0x20 for the first PIC. 0xA0 is the second
	 * Data:	0x21 for the first PIC, 0xA1 for the second
	 */

#define IODELAY		kvga_spin()

	// ICW1 - Reset PIC. Cascaded.
        outb(0x20, 0x11);	IODELAY;
        outb(0xA0, 0x11); 	IODELAY;


	// ICW2 - Remapping
        outb(0x21, 0x20);  /* First PIC (INT 0-7) maps from 0x20 to 0x27 */
	IODELAY;
        outb(0xA1, 0x28);  /* Second PIC (INT 8-15) maps from 0x28 to 0x2F */
	IODELAY;

	// ICW3 - Chaining
        outb(0x21, 0x04); IODELAY;
        outb(0xA1, 0x02); IODELAY;

	// ICW4
        outb(0x21, 0x01); IODELAY;
        outb(0xA1, 0x01); IODELAY;

	pic_mask_all_interrupts();
}

