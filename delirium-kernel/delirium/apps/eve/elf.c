#include <delirium.h>
#include "delibrium/delibrium.h"
#include <elf.h>
#include <paging.h>

#define ELF_MAGIC 0x464C457F
#define ELF_TYPE_EXEC 0x2
#define ELF_TYPE_DYN_EXEC 0x3
#define ELF_PROG_SEG_LOAD 0x1
#define ELF_PROG_SEG_DYNAMIC 0x2
#define ELF_PS_READ 0x4
#define ELF_PS_WRITE 0x2
#define ELF_PS_EXEC 0x1

#if 0
#define DYNAMIC_EXEC_BASE	(void *) 0x81000000
#endif
#define DYNAMIC_EXEC_BASE	(void *) 0x04000000

#undef EVE_ELF_DEBUG

static void *next_exec_base = DYNAMIC_EXEC_BASE;

/*
 * VERY simple elf loader.
 *
 * Make stupid assumptions like linear allocation of segments, in order
 *
 * TODO: Allow for out-of-order sections and memory allocation
 *
 */
void eve_elf_load(void *base) {
	elf_file_header_t *f;
	elf_program_header_t *ph;
	void *alloc_to = (void *)0;
	void *tempstack;
	int i;
	bool dynamic = false;
	void *dyn_base = 0;
	void *dvirt = 0;

	#ifdef EVE_ELF_DEBUG
	print("(Debugging eve ELF loader)\n");
	#endif

	f = base;
	if (f->magic != ELF_MAGIC) { print("Bad ELF magic found"); return; }
	if (f->type != ELF_TYPE_EXEC && f->type != ELF_TYPE_DYN_EXEC) { 
		printf("Unknown ELF executable type (%x)\n", f->type); 
		return;
	}
	if (f->type == ELF_TYPE_DYN_EXEC) { 
		//if (next_exec_base == 0) next_exec_base = DYNAMIC_EXEC_BASE; // BSS
	#ifdef EVE_ELF_DEBUG
		print("Dynamic executable\n");
		printf("next_exec_base: 0x%x\n", next_exec_base);
	#endif
		dyn_base = next_exec_base;
		dynamic = true;
	}

	for (i=0; i<(f->ph_entry_count); i++) {
		ph = base + f->program_head_offset + (f->ph_entry_size * i);
		if (dynamic) {
			dvirt = dyn_base + (u_int32_t) (ph->virtual_addr);
		}

		if (ph->segment_type == ELF_PROG_SEG_DYNAMIC) {
			#ifdef EVE_ELF_DEBUG
			print("Found dynamic section\n");

			/* Show what we are loading if debugging */
			if (ph->flags & ELF_PS_READ) print("R");
			if (ph->flags & ELF_PS_WRITE) print("W");
			if (ph->flags & ELF_PS_EXEC) print("X");
			printf("D| 0x%x(0x%x), mem 0x%x, dsk 0x%x, aln 0x%x\n",
				ph->phys_addr, ph->virtual_addr, ph->mem_size, 
				ph->file_size, ph->alignment);
			if (dynamic) printf("  `- virt 0x%x -> 0x%x\n", ph->virtual_addr, dvirt);

			#endif
			if (! dynamic) {
				print("Strange... Found DYNAMIC section in non dynamic executable. Ignoring.\n");
			} else if (alloc_to == 0) {
				print("Error! Found DYNAMIC section before code loaded. Not loading ELF.\n");
				return;
			} else {
				/* Update symbols */
				elf_dynamic_header_t * dynhead = base + ph->offset;
				elf_relocate_entry_t * reloc_entries = NULL;
				u_int32_t reloc_table_size = 0;

				/* Find the rel table */
				while (dynhead->tag != ELF_DT_NULL) {
					if ((void *)dynhead >= (void *)(base + ph->offset + ph->file_size)) {
						print("Overflowed dynamic section. Probably corrupt ELF. Loading anyway.\n");
						break;
					}
					switch (dynhead->tag) {
					  case ELF_DT_REL:
						#ifdef EVE_ELF_DEBUG
						printf(" D| REL table at offset 0x%x (0x%x absolute)\n", dynhead->dyn_u.val , base + dynhead->dyn_u.val);
						printf("OR! REL table at offset 0x%x (0x%x absolute)\n", dynhead->dyn_u.val , dyn_base+ dynhead->dyn_u.val);
						#endif
						//reloc_entries = base + dynhead->dyn_u.val;
						reloc_entries = dyn_base + dynhead->dyn_u.val;
						break;
					  case ELF_DT_RELSZ:
						reloc_table_size = dynhead->dyn_u.val;
						#ifdef EVE_ELF_DEBUG
						printf(" D| REL table size: 0x%x bytes\n", reloc_table_size);
						#endif
						break;
					}
					++dynhead;
				}

				/* Do the symbol updates */
				elf_relocate_entry_t * reloc_entries_tail = ((void *) reloc_entries)  + reloc_table_size;
				while (reloc_entries < reloc_entries_tail) {
					if (reloc_entries->info == ELF_RELOCATE_RELATIVE) {
						#ifdef EVE_ELF_DEBUG
						void * rel = reloc_entries->offset;
						u_int32_t * ptr = ( (void *) dyn_base + (int32_t) rel );
						//printf("(@0x%x>0x%x 0x%x>0x%x) ", rel, dyn_base+(int32_t)rel, *ptr, *ptr + (int32_t)dyn_base);
						printf("(@0x%x>0x%x 0x%x>0x%x) ",
							reloc_entries->offset, dyn_base + (int32_t) reloc_entries->offset,
							*(int32_t *)(dyn_base + (int32_t)reloc_entries->offset), 
							*(int32_t *)(dyn_base + (int32_t)reloc_entries->offset) + (int32_t)dyn_base);
						#endif
						//*ptr = (int32_t) dyn_base + *ptr;
						reloc_entries->offset = dyn_base + (int32_t) reloc_entries->offset;
						*(int32_t *)(reloc_entries->offset) += (int32_t)dyn_base;
					}
					else {
					#ifdef EVE_ELF_DEBUG
						printf("   Found unknown REL table entry %d\n", reloc_entries->info);
					#endif
						printf("   Bailing!");
						return;
					}
					++reloc_entries;
				}
			}
		}
	

		if (ph->segment_type == ELF_PROG_SEG_LOAD) {
			int pages_needed = ((ph->mem_size) / PAGE_SIZE) + 1;
			void * pages[pages_needed];
			void *abase = ph->virtual_addr;
			int i;

			if (dynamic) abase = dvirt;

			/* Show what we are loading if debugging */
			if (ph->flags & ELF_PS_READ) print("R");
			if (ph->flags & ELF_PS_WRITE) print("W");
			if (ph->flags & ELF_PS_EXEC) print("X");
			printf("| 0x%x(0x%x), mem 0x%x, dsk 0x%x, aln 0x%x\n",
				ph->phys_addr, ph->virtual_addr, ph->mem_size, 
				ph->file_size, ph->alignment);
			if (dynamic) printf("  `- virt 0x%x -> 0x%x\n", ph->virtual_addr, dvirt);

			/* Don't double allocate pages, no matter how 
			 * stupid and broken the ELF header is */
			while (abase < alloc_to) {
				pages_needed--;
				abase += PAGE_SIZE;
			}
			alloc_to = abase + (PAGE_SIZE * pages_needed) - 1;
			getpages(pages_needed, pages);
			for (i = 0; i < pages_needed; i++) {
				add_to_pagedir(get_current_page_dir(), abase, pages[i]);
	       		        abase += PAGE_SIZE;
		        }

			memcpy(dynamic ? dvirt : ph->virtual_addr, base+(ph->offset), 
					ph->file_size);

			if (ph->file_size < ph->mem_size) {
				memset((dynamic ? dvirt : ph->virtual_addr)+(ph->file_size),
					0, (ph->mem_size - ph->file_size));
			}
		}
	}

	if (dynamic)
		next_exec_base = (alloc_to + PAGE_SIZE) - ((u_int32_t) alloc_to % PAGE_SIZE);
	
	/* Splinter, but give the tempstack, so we have to yield to make
	 * sure that it is freed again before we are called again in this
	 * thread */

	tempstack = getpage();

	void *entrypoint = f->entry;
	if (dynamic) {
		entrypoint = dyn_base + (u_int32_t) f->entry;
	}
	#ifdef EVE_ELF_DEBUG
		printf("eve_elf_load: splintering to entrypoint 0x%x\n", entrypoint);
	#endif
	tempstack += PAGE_SIZE;
	tempstack -= sizeof(void *);
	splinter(entrypoint, tempstack);
	#if 0
	/*** Ran into problems doing this where the tempstack was freed before we got to change the stack. ***/
	yield();
	freepage(tempstack);
	#endif
}
