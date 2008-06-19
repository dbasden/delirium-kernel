#ifndef __ELF_H
#define __ELF_H

/*
 * ELF binary information for Delirium
 *
 * Caveats: Only ELF 32 bit stuff, and only the tiny bits we need
 */

#include <delirium.h>

typedef struct {
	u_int32_t	magic;
	u_int32_t	head;
	u_int32_t	res1;
	u_int32_t	res2;
	u_int16_t	type;
	u_int16_t	arch;
	u_int32_t	elf_version;
	void *		entry;
	u_int32_t	program_head_offset; // within file
	u_int32_t	section_head_offset; // within file
	u_int32_t	flags;
	u_int16_t	headsize;
	u_int16_t	ph_entry_size; // Program header
	u_int16_t	ph_entry_count;
	u_int16_t	sh_entry_size; // Section header
	u_int16_t	sh_entry_count;
	u_int16_t	strtab_index;
} elf_file_header_t;

typedef struct {
	u_int32_t	segment_type;
	u_int32_t	offset; // within file
	void *		virtual_addr;
	void *		phys_addr;
	u_int32_t	file_size; // size in file
	u_int32_t	mem_size; // size in memory
	u_int32_t	flags;
	u_int32_t	alignment;
} elf_program_header_t;

typedef struct {
	u_int32_t	name;
	u_int32_t	type;
	u_int32_t	flags;
	void *		addr;
	void *		offset;
	u_int32_t	size;
	u_int32_t	link;
	u_int32_t	info;
	u_int32_t	align;
	u_int32_t	entrysize;
} elf_section_header_t;

/*
 * stuff that can be in section header flags
 */

#define ELF_WRITE	1
#define ELF_ALLOC	2
#define ELF_EXEC	4
#define ELF_MERGE	8
#define	ELF_STRING	16
#define ELF_INFO	32
#define ELF_ORDER	64
#define ELF_WEIRDOS	128
#define ELF_GROUP	256
#define ELF_TLS		512

#endif // __ELF_H
