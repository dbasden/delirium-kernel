/*
 * arch/xen/hypercall.S: Wrappers to Xen hypercalls for DeLiRiuM
 *
 * This is a simple wrapper for the code in the Xen hypervisor's
 * hypercall_page. The hypervisor (as of xen-3.0.3) expects a guest
 * to call-by-register, while the DeLiRiuM kernel passes on the stack.
 *
 * - The hypercall number is stored in %eax, while the argument(s) to the
 *   call are in %ebx, %ecx, %edx, %esi, %edi for arguments 1-5 respectively.
 *  The argument registers are clobbered, and the return value stored in %eax
 *
 * - The Xen header comments in xen/public/arch-x86_32.h imply that only the
 *   registers that have arguments for that hypercall are clobbered, but I'm going to
 *   assume that they all regardless.
 *
 * The wrappers in this file depend on the __xen_hypercall_* symbols exported
 * in boot.S /hypercall_gate.S 
 *
 * Copyright (c)2007 David Basden
 */

#define __ASSEMBLY__

	.text

	.macro WRAP_HYPERCALL callname
	.globl \callname
\callname:
	pushl	%ebp
	movl	%esp,%ebp
	pushal	// EAX ECX EDX EBX ESP EBP ESI EDI
	movl	8(%ebp), %ebx
	movl	12(%ebp), %ecx
	movl	16(%ebp), %edx
	movl	18(%ebp), %esi
	movl	22(%ebp), %edi
	call	__xen_hypercall_gate_\callname
	popal
	movl	%ebp,%esp
	popl	%ebp
	ret
	.endm

WRAP_HYPERCALL set_trap_table
WRAP_HYPERCALL mmu_update
WRAP_HYPERCALL set_gdt
WRAP_HYPERCALL stack_switch
WRAP_HYPERCALL set_callbacks
WRAP_HYPERCALL fpu_taskswitch
WRAP_HYPERCALL sched_op_compat
WRAP_HYPERCALL platform_op
WRAP_HYPERCALL set_debugreg
WRAP_HYPERCALL get_debugreg
WRAP_HYPERCALL update_descriptor
WRAP_HYPERCALL memory_op
WRAP_HYPERCALL multicall
WRAP_HYPERCALL update_va_mapping
WRAP_HYPERCALL set_timer_op
WRAP_HYPERCALL event_channel_op_compat
WRAP_HYPERCALL xen_version
WRAP_HYPERCALL console_io
WRAP_HYPERCALL physdev_op_compat
WRAP_HYPERCALL grant_table_op
WRAP_HYPERCALL vm_assist
WRAP_HYPERCALL update_va_mapping_otherdomain
WRAP_HYPERCALL iret
WRAP_HYPERCALL vcpu_op
WRAP_HYPERCALL set_segment_base
WRAP_HYPERCALL mmuext_op
WRAP_HYPERCALL acm_op
WRAP_HYPERCALL nmi_op
WRAP_HYPERCALL sched_op
WRAP_HYPERCALL callback_op
WRAP_HYPERCALL xenoprof_op
WRAP_HYPERCALL event_channel_op
WRAP_HYPERCALL physdev_op
WRAP_HYPERCALL hvm_op
WRAP_HYPERCALL sysctl
WRAP_HYPERCALL domctl
WRAP_HYPERCALL kexec_op
