#ifndef __DELIBRIUM_TIMER_H
#define __DELIBRIUM_TIMER_H

#include <delirium.h>


/* 
 * On Pentium and above CPUs, read the 64bit TSC MSR,
 * which is incremented once per clock tick.
 */
#ifdef ARCH_i386
u_int64_t rdtsc();
#endif

#endif
