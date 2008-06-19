#ifdef _C
#include <delirium.h>
#endif

/* multiboot.h - the header for Multiboot */
/* modified to work with delirium and not make assumptions
 * about sizeof(unsigned long) */
/* Copyright (C) 1999, 2001  Free Software Foundation, Inc.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Macros. */

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* The flags for the Multiboot header. */
#ifdef __ELF__
# define MULTIBOOT_HEADER_FLAGS         0x00000003
#else
# define MULTIBOOT_HEADER_FLAGS         0x00010003
#endif

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

/* The size of our stack (16KB). */
#define STACK_SIZE                      0x4000

/* C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

#ifndef ASM
/* Do not include here in boot.S. */

/* Types. */

/* The Multiboot header. */
typedef struct multiboot_header
{
  u_int32_t magic;
  u_int32_t flags;
  u_int32_t checksum;
  u_int32_t header_addr;
  u_int32_t load_addr;
  u_int32_t load_end_addr;
  u_int32_t bss_end_addr;
  u_int32_t entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
  u_int32_t tabsize;
  u_int32_t strsize;
  u_int32_t addr;
  u_int32_t reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
  u_int32_t num;
  u_int32_t size;
  u_int32_t addr;
  u_int32_t shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info
{
  u_int32_t flags;
  u_int32_t mem_lower;
  u_int32_t mem_upper;
  u_int32_t boot_device;
  u_int32_t cmdline;
  u_int32_t mods_count;
  u_int32_t mods_addr;
  union
  {
    aout_symbol_table_t aout_sym;
    elf_section_header_table_t elf_sec;
  } u;
  u_int32_t mmap_length;
  u_int32_t mmap_addr;
} multiboot_info_t;

/* The module structure. */
typedef struct module
{
  u_int32_t mod_start;
  u_int32_t mod_end;
  u_int32_t string;
  u_int32_t reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map
{
  u_int32_t size;
  u_int32_t base_addr_low;
  u_int32_t base_addr_high;
  u_int32_t length_low;
  u_int32_t length_high;
  u_int32_t type;
} memory_map_t;

#endif /* ! ASM */


