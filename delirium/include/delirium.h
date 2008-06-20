#ifndef _DELIRIUM_H
#define _DELIRIUM_H

#define	DELIRIUM_MAJOR_VER	0
#define	DELIRIUM_MINOR_VER	6

#define	NULL	((void *)0x0)

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
typedef unsigned char 	u_int8_t;
typedef unsigned short 	u_int16_t;
typedef unsigned int 	u_int32_t;
typedef unsigned long long u_int64_t;
typedef char		int8_t;
typedef short		int16_t;
typedef int		int32_t;
typedef long long	int64_t;

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
extern void kill_current_thread();
extern int splinter(void *start(), void *stackp);
extern void add_interrupt_handler(u_int16_t offset, void (*handler)(void));

#endif

#endif /* ifndef ASM */
#endif /* ifndef _DELIRIUM_H */
