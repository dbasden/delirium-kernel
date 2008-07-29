#ifndef _KLIB_H
#define _KLIB_H

#include <delirium.h>

#ifdef DEBUG
#define kdebug(_msg)	kprintf("DEBUG: " _msg "\n")
#else
#define	kdebug(_msg)
#endif

#ifndef ARCH_lucid
void kprintf(char *, ...);
void kprint(char *);
void kmemcpy(char *, char *, unsigned int);
void kmemset(char *, char, unsigned int);
void kmemstamp(char *, char *, unsigned int, unsigned int);
int kstrcmp(char *, char *);

#endif

#ifdef ARCH_lucid
#include "lucid.h"
#endif

#ifndef ARCH_i386
#define kpanic	kpanic_generic
#endif

void kpanic();

#endif
