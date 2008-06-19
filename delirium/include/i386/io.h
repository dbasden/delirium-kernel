#ifndef __I386_IO_H
#define __I386_IO_H

#include <delirium.h>

// inline void outb(u_int16_t, char);
//inline unsigned char inb(u_int16_t);

#define outb(_port, _byte)      asm volatile ( "outb %1, %0" : : "Nd" ((u_int16_t)(_port)), "a" ((u_int8_t)(_byte)))

#define inb(_port)\
	({ u_int8_t __ret; \
	 asm volatile ( "inb %1, %0" : "=a" (__ret) : "Nd" ((u_int16_t)(_port)));\
	 __ret; })


#endif
