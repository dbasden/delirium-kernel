#include <delirium.h>
#include <i386/io.h>

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
	outb(baseport + 3, 0x80);	/* Divisor Latch Enable */
	outb(baseport, (divisor % 0xff));
	outb(baseport + 1, ((divisor >> 8) % 0xff));
	outb(baseport + 3, 0x03);	/* 8N1 */
	outb(baseport + 2, 0xc7);	/* Use FIFOs. Clear, and set IRQ trigger to 14 */
	outb(baseport + 4, 0x03);	/* DTS/RTS */
	outb(baseport + 1, 0x01);	/* Enable Interrupts */
}

inline char read_serial(u_int16_t baseport) {
	char c;

	do { yield(); c = inb(baseport + 5); } while ((c & 1) == 0);
	c = inb(baseport);
	return c;
}

inline void send_serial(u_int16_t baseport, char ch) {
	char c;

	do { c = inb(baseport + 5); } while ((c & 0x20) == 0);
	outb(baseport, ch);
}
