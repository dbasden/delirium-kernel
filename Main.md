# Introduction #

DeLiRiuM is a 32-bit "toy" exo/micro-kernel hybrid operating system, with the aim of developing and testing unconventional interfaces between OS and userspace. It aims to move away from a POSIX.1-like interface, without sacrificing modern OS features. Development currently targets IA32 (x86), the Xen hypervisor and UNIX userspace.

See http://davidb.ucc.asn.au/delirium/ for the main project page.

# Build requirements #

  * GCC _(although the code should actually be ANSI C, the latest build hasn't been tested with it)_
  * GNU Make
  * GNU as (gas)