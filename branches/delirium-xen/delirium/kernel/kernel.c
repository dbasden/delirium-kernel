/*
 * Delirium.
 *
 * Main C entry point for kernel.
 */

#include <delirium.h>
#include <cpu.h>		
#include <multiboot.h>
#include <elf.h>
#include "multitask.h"
#include "setup.h"
#include "klib.h"
#include "frameherd.h"
#include "kthump.h"
#include "soapbox.h"
#include "rant.h"
#include "ramtree.h"
#include "kpools.h"

#define MAX_INIT_APPS	4

struct init_app { void *base; size_t size; };
static struct init_app	initial_apps[MAX_INIT_APPS]; 
static size_t initial_app_count;

extern void *delirium_start_ptr;
extern void *delirium_end_ptr;

/*
 * Setup the frame herder from multiboot information
 *
 * TODO: Move to arch/i386
 */
static void setup_framemalloc(multiboot_info_t *mb) {
	memory_map_t *mm; 
	elf_section_header_table_t *elfht;
	elf_section_header_t *esh;
	int i;
	size_t reserved = 0;
	int kused = 0;

	init_herder();

	if ( (!mb) || !(mb->flags & (1 << 6)) ) {
		kprint("\nNo multiboot memory map provided. Denied.\n");
		kprint("Stupidly making something up instead...\n");
		add_mem_to_herd((void *)0, 639 * 1024);
		add_mem_to_herd((void *)(1024 * 1024), 3 * 1024 * 1024);
	} else {
		mm = (void *) mb->mmap_addr;

		for  (	mm = (void *) mb->mmap_addr;
			(void *)mm < (void *)(mb->mmap_addr + mb->mmap_length);
			mm = (void *) mm + mm->size + sizeof(mm->size)
			) {
			if (mm->type != 1) continue;// Only interested in RAM
			if (mm->base_addr_high) continue;// Too high for IA32

			kprintf("(+%uk)", (mm->length_low) >> 10);

			add_mem_to_herd((void *) mm->base_addr_low,
					(size_t) mm->length_low);
		}
	}

	/* 
	 * Overwriting the herd index would be bad, so
	 * reserve it.
	 */
	take_mem_from_herd((void *) HERD_BASE, HERD_INDEX_SIZE);

	/*
	 * Find all the ELF sections in kernel space, reserve them
	 *
	 * TODO: Have temporary code and data sections that can be
	 * thrown away after initial boot (including this code)
	 */

	if ( !(mb) || !(mb->flags & (1 << 5) ) ) {
		/* Use symbols hacked in from link to figure out
		 * where the kernel is loaded */
		take_mem_from_herd(delirium_start_ptr,
		(size_t)(&delirium_end_ptr) - (size_t)delirium_start_ptr);
#if 0
		kprint("\nNo ELF section information provided. Denied.\n");
		kprint("Stupidly making something up instead...\n");
		take_mem_from_herd((void *)0x100000, 128 * 1024);
#endif
	} else {
		elfht = &((mb->u).elf_sec);
		for (i=reserved=0; i < (elfht->num); i++) {
			esh = (void *) elfht->addr + (i * elfht->size);

			if (~(esh->flags) & ELF_ALLOC) 
				continue;

			reserved += esh->size;
			take_mem_from_herd(esh->addr, esh->size);
		}
		kprintf("(-%uk)", (reserved/FRAME_SIZE+((reserved%FRAME_SIZE)?1:0))* 4);
	}

	/*
	 * look for modules
	 */
	initial_app_count = 0;
	if ( mb && mb->flags & (1 << 3) ) {
		module_t * mod;
		int i,s;

		initial_app_count = mb->mods_count;

		mod = (void *)(mb->mods_addr);
		for (i=0; i<mb->mods_count; i++) {
			s = mod[i].mod_end - mod[i].mod_start;
			take_mem_from_herd((void *)(mod[i].mod_start), s);
			initial_apps[i].base = (void*)(mod[i].mod_start);
			kused +=  (s/FRAME_SIZE+((s%FRAME_SIZE)?1:0))* 4;
		}
		kprintf("(-%uk)", kused);

	}
	
}

#ifdef ARCH_xen
extern int set_debugreg(int reg, unsigned long value);
extern int xen_version(int cmd);
extern int console_io(int cmd, int count, char *str);
#endif

// Entry point from ASM
void cmain(u_int32_t multi_magic, void *multi_addr) {
	int i;

#ifdef ARCH_i386
#ifndef ARCH_xen
	if (multi_magic != (MULTIBOOT_BOOTLOADER_MAGIC)) {
		kprintf("Not called by a multiboot loader (magic is %08x and should be %08x)\nThings could get really weird from here on in.\n", 
			multi_magic, MULTIBOOT_BOOTLOADER_MAGIC);
		multi_addr = 0;
	}
#endif
#endif


#ifdef ARCH_xen
	set_debugreg(0, 0xdeadbeef);
	set_debugreg(1, 0xfeedbead);
	set_debugreg(2, 0xc0c1f00f);
	i = xen_version(0);

	console_io(0, 9, "DeLiRiuM\n");
	i = xen_version(0);
	kprintf("Xen version: %d\n", i);
	return;
#endif

	kprintf("DeLiRiUM v%d.%d %% Still not king.\n\n", DELIRIUM_MAJOR_VER, DELIRIUM_MINOR_VER);

	kprint("Configuring:");

	kprint(" memory");;
	setup_memory();


	kprint(", interrupts");
	setup_interrupts();

#ifdef ENABLE_GDB_STUB
	kprintf(", gdbstub");
	extern void set_debug_traps();
	set_debug_traps();
#endif

	kprint(", frameherd");
	setup_framemalloc(multi_addr);

	kprint(",\n             paging");
	setup_paging();

	kprint(", pools");
	setup_pools();

	kprint(", tasks");
	setup_tasks();

	kprint(", ipc");
	setup_soapbox();
	init_rant();

	kprint(", kthump");
	setup_kthump();

	kprint(". (done)\n\n");
			
	// Initial apps may depend on ramtree existing. Load it first
	//
	for (i=0; i<initial_app_count; i++)
		if (*((u_int32_t *)initial_apps[i].base) == RAMTREE_MUNDANE)
			load_ramtree(initial_apps[i].base);

#ifdef ENABLE_GDB_STUB
	extern void breakpoint();
	breakpoint();
#endif

#ifdef ENABLE_DEBUG_THREAD
	extern void setup_debug_listener();
	setup_debug_listener();
#endif

	// Assume any other files are to be ELF loaded.
	//
	for (i=0; i<initial_app_count; i++) 
		if (*((u_int32_t *)initial_apps[i].base) != RAMTREE_MUNDANE) 
			elf_load(initial_apps[i].base, initial_apps[i].size);

	exit_kthread();
	//for (;;) yield();
}

