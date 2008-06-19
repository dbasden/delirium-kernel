#ifndef __GDT_H
#define __GDT_H

#include <delirium.h>

/* Global Descriptor Table Offsets */
#define	GDT_KERNEL_CODE		0x08
#define GDT_KERNEL_DATA		0x10
#define GDT_KERNEL_TASKS	0x18

/* GDT flags - "access flags" */
#define GDT_DPL_0		0x00
#define GDT_DPL_1		0x20
#define GDT_DPL_2		0x40
#define GDT_DPL_3		0x60

#define GDT_TYPE_TSS_AVAIL	0x89
#define GDT_TYPE_TSS_BUSY	0x8a

#define GDT_SEG_PRESENT		0x80
#define TSS_DESC_BUSY		0x02

/* GDT flags - high nibble to limit */ 
#define GDT_GRANULARITY		0x80
#define	TSS_DESC_AVAIL		0x10

#ifndef ASM
#define _pak		__attribute__ ((packed))

typedef struct {
	u_int16_t	limit_low	_pak;
	u_int16_t	base_low	_pak;
	u_int8_t	base_16_23	_pak;
	u_int8_t	access_flags	_pak;
	u_int8_t	flags_limit_hi	_pak;
	u_int8_t	base_24_31	_pak;
} gdt_entry_t;

typedef struct {
	u_int16_t	gdt_entries	_pak;
	gdt_entry_t *	gdt		_pak;
} gdt_desc_t;

gdt_desc_t	gdt_descriptor;

#endif

#endif
