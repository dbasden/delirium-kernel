#include <delirium.h>
#include "klib.h"
#include "i386/interrupts.h"
#include "i386/io.h"

/* From the interweb */

#define com1 0x3f8 
#define com2 0x2f8

#define combase com1

void init_serial(void)
{
    outb(combase+3,inb(combase + 3) | 0x80);
    outb(combase, 12); /* 9600 bps, 8-N-1 */
    outb(combase+1, 0);
    outb(combase+3, inb(combase + 3) & 0x7f);
}

int getDebugChar(void)
{
    while (!(inb(combase + 5) & 0x01));
    return inb(combase);
}

void putDebugChar(int ch)
{
    while (!(inb(combase + 5) & 0x20));
    outb(combase, (char) ch);
}

void exceptionHandler(int exc, void *addr)
{
	add_handler(exc, addr);
}

void flush_i_cache(void)
{
   __asm__ __volatile__ ("jmp 1f\n1:");
}
