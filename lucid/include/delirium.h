#ifndef _DELIRIUM_H
#define _DELIRIUM_H

#define	DELIRIUM_MAJOR_VER	0
#define	DELIRIUM_MINOR_VER	6

#ifndef NULL
#define	NULL	((void *)0x0)
#endif

#define BIT(pos)	(1<<(pos))

#define bool		int
#define true		-1
#define false		0

#ifndef ARCH
#ifndef ARCH_i386
#define ARCH_i386
#endif
#endif

#ifndef ASM
/* TODO: Move to types.h */
#ifndef _LUCID_INT_TYPES_DEFINED
#define _LUCID_INT_TYPES_DEFINED
typedef unsigned char 	u_int8_t;
typedef unsigned short 	u_int16_t;
typedef unsigned int 	u_int32_t;
typedef unsigned long long u_int64_t;
typedef char		int8_t;
typedef short		int16_t;
typedef int		int32_t;
typedef long long	int64_t;
#endif

#ifdef ARCH_lucid
#include "lucid.h"
#endif

#ifdef size_t
#undef size_t
#endif
#define size_t		u_int32_t

#ifndef KERNEL

// TODO: Clean up the .h files some more, especially
// distinguishing user and kernel namespaces
//
// Here are some of the exported calls

extern void freepages(int, void **);
extern int getpages(int, void **);
extern void *herdpages(int, void **);
extern int getpages(int, void **);
extern void *getpage();
extern int getpages(int, void **);
extern void freepages(int, void **);
extern void freepage(void *);
extern void freepages(int, void **);
extern void mask_interrupt(char intr);
extern void unmask_interrupt(char intr);
extern void yield();
extern void print(char *);
extern int splinter(void *start(), void *stackp);
extern void add_interrupt_handler(u_int16_t offset, void (*handler)(void));
extern void add_c_interrupt_handler(u_int32_t hwirq, void (*handler)(void));
extern void remove_interrupt_handler(u_int8_t hw_int);

#endif

#endif /* ifndef ASM */
#endif /* ifndef _DELIRIUM_H */
