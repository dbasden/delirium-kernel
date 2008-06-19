#include <delirium.h>
#include <cpu.h>
#include "klib.h"
#include "elf.h"
#include "paging.h"
#include "multitask.h"

#define ELF_MAGIC 0x464C457F
#define ELF_TYPE_EXEC 0x2
#define ELF_TYPE_DYN_EXEC 0x3
#define ELF_PROG_SEG_LOAD 0x1
#define ELF_PS_READ 0x4
#define ELF_PS_WRITE 0x2
#define ELF_PS_EXEC 0x1

/*
 * VERY simple elf loader.
 *
 * Make stupid assumptions like linear allocation of segments, in order
 */
void elf_load(void *base, size_t size) {
	elf_file_header_t *f;
	elf_program_header_t *ph;
	void *alloc_to = (void *)0;
	int i;

	f = base;
	if (f->magic != ELF_MAGIC) { kprint("Bad ELF magic found"); return; }
	if (f->type != ELF_TYPE_EXEC && f->type != ELF_TYPE_DYN_EXEC) { 
		kprintf("Unknown ELF executable type (%x)\n", f->type); 
		return;
	}

	for (i=0; i<(f->ph_entry_count); i++) {
		ph = base + f->program_head_offset + (f->ph_entry_size * i);

		if (ph->segment_type == ELF_PROG_SEG_LOAD) {
			int pages_needed = ((ph->mem_size) / PAGE_SIZE) + 1;
			void * pages[pages_needed];
			void *abase = ph->virtual_addr;
			int i;

			/* Show what we are loading if debugging */
			if (ph->flags & ELF_PS_READ) kprint("R");
			if (ph->flags & ELF_PS_WRITE) kprint("W");
			if (ph->flags & ELF_PS_EXEC) kprint("X");
			kprintf("elf:\t0x%x(0x%x), mem 0x%x, dsk 0x%x, aln 0x%x\n",
				ph->phys_addr, ph->virtual_addr, ph->mem_size, 
				ph->file_size, ph->alignment);

			/* Don't double allocate pages, no matter how 
			 * stupid and broken the ELF header is */
			while (abase < alloc_to) {
				pages_needed--;
				abase += PAGE_SIZE;
			}
			alloc_to = abase + (PAGE_SIZE * pages_needed) - 1;
			kgetpages(pages_needed, pages);
			for (i = 0; i < pages_needed; i++) {
		                paging_add_to_dir(kernel_page_dir, abase, pages[i]);
	       		        abase += PAGE_SIZE;
		        }


			kmemcpy(ph->virtual_addr, base+(ph->offset), 
					ph->file_size);

			if (ph->file_size < ph->mem_size) {
				kmemset((ph->virtual_addr)+(ph->file_size),
					0, (ph->mem_size - ph->file_size));
			}
		}
	}
	
	/* Splinter, but give the tempstack, so we have to yield to make
	 * sure that it is freed again before we are called again in this
	 * thread */
	splinter(f->entry, tempstack+PAGE_SIZE);
	yield();
}
