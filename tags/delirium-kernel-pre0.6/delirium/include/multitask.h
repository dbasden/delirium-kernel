#ifndef __MULTITASK_H
#define __MULTITASK_H

#ifdef ARCH_i386
#include "i386/task.h"
#endif

extern int splinter(void *start(), void *stackp);
extern void elf_load(void *, size_t);
extern void *tempstack;
extern thread_t get_thread_info();

#endif
