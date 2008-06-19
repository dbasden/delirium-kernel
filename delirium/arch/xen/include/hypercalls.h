 /*
  * prototypes for access to Xen hypercalls
  * hypercall.S DeLiRiuM access to Xen hypercalls
  *
  * Can ONLY be included as part of boot.S so that we can
  * .set based on HYPERCALL_BASE/hypercall_page at assemble
  * times.
  *
  *  DO NOT ASSEMBLE AND LINK THIS BY IT'S OWN; IT IS INCLUDED
  *  IN arch/xen/boot.S
  *
  * Copyright (c)2007 David Basden --  All Rights Reserved
  */

#define __ASSEMBLY__
#include "xen/public/xen.h"

#define HYPERCALL_GATE(_n) \
.set hypercall_##_n ,  ( hypercall_page + ( __HYPERVISOR_##_n * 32  )) \
; .globl hypercall_##_n;


HYPERCALL_GATE(set_trap_table)
HYPERCALL_GATE(mmu_update)
HYPERCALL_GATE(set_gdt)


#if 0
set_trap_table        0
mmu_update            1
set_gdt               2
stack_switch          3
set_callbacks         4
fpu_taskswitch        5
sched_op_compat       6 /* compat since 0x00030101 */
platform_op           7
set_debugreg          8
get_debugreg          9
update_descriptor    10
memory_op            12
multicall            13
update_va_mapping    14
set_timer_op         15
event_channel_op_compat 16 /* compat since 0x00030202 */
xen_version          17
console_io           18
physdev_op_compat    19 /* compat since 0x00030202 */
grant_table_op       20
vm_assist            21
update_va_mapping_otherdomain 22
iret                 23 /* x86 only */
vcpu_op              24
set_segment_base     25 /* x86/64 only */
mmuext_op            26
acm_op               27
nmi_op               28
sched_op             29
callback_op          30
xenoprof_op          31
event_channel_op     32
physdev_op           33
hvm_op               34
sysctl               35
domctl               36
kexec_op             37

#endif
