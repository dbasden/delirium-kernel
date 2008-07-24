#ifndef __I386_IO_H
#define __I386_IO_H

#include <delirium.h>

#define outb(_port, _byte)      asm volatile ( "outb %1, %0" : : "Nd" ((u_int16_t)(_port)), "qa" ((u_int8_t)(_byte)))

#define outw(_port, _w)      asm volatile ( "outw %1, %0" : : "Nd" ((u_int16_t)(_port)), "Ra" ((u_int16_t)(_w)))

#define outl(_port, _l)      asm volatile ( "outl %1, %0" : : "Nd" ((u_int16_t)(_port)), "a" ((u_int32_t)(_l)))

#define inb(_port) ({ u_int8_t __ret; asm volatile ( "inb %1, %0" : "=qa" (__ret) : "Nd" ((u_int16_t)(_port))); __ret; })
#define inw(_port) ({ u_int16_t __ret; asm volatile ( "inw %1, %0" : "=Ra" (__ret) : "Nd" ((u_int16_t)(_port))); __ret; })
#define inl(_port) ({ u_int32_t __ret; asm volatile ( "inl %1, %0" : "=a" (__ret) : "Nd" ((u_int16_t)(_port))); __ret; })


#endif
