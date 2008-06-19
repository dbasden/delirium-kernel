#include <delirium.h>
#include <i386/io.h>

#if 0
inline void outb(u_int16_t port, char byte) {
        asm volatile ( "outb %1, %0" : : "Nd" (port), "a" (byte));
}

inline unsigned char inb(u_int16_t port) {
	unsigned char byte;

	asm volatile ( "inb %1, %0" : "=a" (byte) : "Nd" (port));
	return byte;
}



#endif
