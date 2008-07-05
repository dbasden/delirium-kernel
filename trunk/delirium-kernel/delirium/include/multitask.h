#ifndef __MULTITASK_H
#define __MULTITASK_H

#ifdef ARCH_i386
#include "i386/task.h"
#endif

extern int splinter(void *start(), void *stackp);
extern void elf_load(void *, size_t);
extern void *tempstack;
extern thread_t get_thread_info();
extern void set_thread_state(thread_state_t new_state);
extern void wake_thread(size_t thread_id);
extern void await();

#endif
