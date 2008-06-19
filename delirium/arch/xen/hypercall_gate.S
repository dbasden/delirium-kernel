 /*
  * hypercall.S: DeLiRiuM access to Xen hypercall page
  *
  *
  *  DO NOT ASSEMBLE AND LINK THIS BY IT'S OWN; IT IS INCLUDED
  *  IN arch/xen/boot.S
  *
  * Can ONLY be included as part of boot.S so that we can
  * .set based on HYPERCALL_BASE/hypercall_page at assemble
  * times.
  *
  * Basically, this code gives us global symbols for DeLiRiuM's kernel
  * so that we don't have to include magic numbers or xen/public/xen.h
  * every time we want to make a hypercall.
  *
  * We don't actually implement the hypercall calling conventions here,
  * just give the points to call/jmp to. At this point, the code at these
  * symbols (filled in by the hypervisor NOT DeLiRiuM) does this:
  *
  *	mov	HYPERCALL_NUMBER, %eax
  *	int	0x82
  *	ret
  *
  * and args are passed by register in EBX, ECX and EDX as needed before
  * getting to the hypercall_page entry, with all general purpose registers
  * clobbered. Although this call-convention is current as at Xen 3.0.3 for
  * x86-32, it can't be guaranteed. 
  *
  * Copyright (c)2007 David Basden --  All Rights Reserved
  */

#define __ASSEMBLY__
#include "xen/public/xen.h"

#define HYPERCALL_GATE(_n) \
.set __xen_hypercall_gate_##_n ,  ( hypercall_page + ( __HYPERVISOR_##_n * 32  )) \
; .globl __xen_hypercall_gate_##_n;

HYPERCALL_GATE(set_trap_table)
HYPERCALL_GATE(mmu_update)
HYPERCALL_GATE(set_gdt)
HYPERCALL_GATE(stack_switch)
HYPERCALL_GATE(set_callbacks)
HYPERCALL_GATE(fpu_taskswitch)
HYPERCALL_GATE(sched_op_compat)
HYPERCALL_GATE(platform_op)
HYPERCALL_GATE(set_debugreg)
HYPERCALL_GATE(get_debugreg)
HYPERCALL_GATE(update_descriptor)
HYPERCALL_GATE(memory_op)
HYPERCALL_GATE(multicall)
HYPERCALL_GATE(update_va_mapping)
HYPERCALL_GATE(set_timer_op)
HYPERCALL_GATE(event_channel_op_compat)
HYPERCALL_GATE(xen_version)
HYPERCALL_GATE(console_io)
HYPERCALL_GATE(physdev_op_compat)
HYPERCALL_GATE(grant_table_op)
HYPERCALL_GATE(vm_assist)
HYPERCALL_GATE(update_va_mapping_otherdomain)
HYPERCALL_GATE(iret)
HYPERCALL_GATE(vcpu_op)
HYPERCALL_GATE(set_segment_base)
HYPERCALL_GATE(mmuext_op)
HYPERCALL_GATE(acm_op)
HYPERCALL_GATE(nmi_op)
HYPERCALL_GATE(sched_op)
HYPERCALL_GATE(callback_op)
HYPERCALL_GATE(xenoprof_op)
HYPERCALL_GATE(event_channel_op)
HYPERCALL_GATE(physdev_op)
HYPERCALL_GATE(hvm_op)
HYPERCALL_GATE(sysctl)
HYPERCALL_GATE(domctl)
HYPERCALL_GATE(kexec_op)
