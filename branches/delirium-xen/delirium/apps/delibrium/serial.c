#include <delirium.h>
#include <i386/io.h>
#include "serial.h"

#define SERIAL_IRQ_EVERY_1_WORD		0x00
#define SERIAL_IRQ_EVERY_4_WORDS	0x40
#define SERIAL_IRQ_EVERY_8_WORDS	0x80
#define SERIAL_IRQ_EVERY_14_WORDS	0xc0

#define SERIAL_IRQ_RX_DATA_AVAIL	0x01
/*
 * Setup a 16[45]50 UART
 *
 * This will also enable interrupts on the serial port; Register an
 * interrupt handler as well.
 */
void init_serial(u_int16_t baseport, size_t speed) {
	int divisor;

	divisor = 115200 / speed;

	outb(baseport + 1, 0);		/* Disable interrupts */
	outb(baseport + 1, SERIAL_IRQ_RX_DATA_AVAIL);	
	outb(baseport + 3, 0x80);	/* Divisor Latch Enable */
	outb(baseport, (divisor % 0xff));
	outb(baseport + 1, ((divisor >> 8) % 0xff));
	outb(baseport + 3, 0x03);	/* 8N1 */
	outb(baseport + 2, 0x07 | SERIAL_IRQ_EVERY_1_WORD);	/* Use FIFOs. Clear, and set IRQ trigger to every word */
	outb(baseport + 4, 0x03);	/* DTS/RTS */
	outb(baseport + 1, 0x01);	/* Enable Interrupts */
}

/* consume and discard buffer */
inline void consume_serial_buffer(u_int16_t baseport)  {
	u_int8_t c;
	while (( (c = inb(baseport + 5)) & 1 ) != 0)
		c = inb(baseport);
}

/* Don't use this. Really. */
inline char blocking_read_serial(u_int16_t baseport) {
	char c;

	do { c = inb(baseport + 5); } while ((c & 1) == 0);
	c = inb(baseport);
	return c;
}

inline void blocking_send_serial(u_int16_t baseport, char ch) {
	char c;

	do { c = inb(baseport + 5); } while ((c & 0x20) == 0);
	outb(baseport, ch);
}
