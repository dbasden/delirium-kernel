/*
 * Delirium - kvga.c
 *
 * Simple VGA console output for boot time
 *
 * Note: Most of these calls do no bounds checking.
 *
 * Copyright (c)2003-2005 David Basden
 */

#include <assert.h>
#include <cpu.h>
#include "klib.h"
#include "kvga.h"


volatile Semaphore INIT_SEMAPHORE(vga_print_s);

static unsigned char volatile xpos = 0;
static unsigned char volatile ypos = (VGA_LINES-1);

static char *vidmem = VGA_BASE;

char *spinner = "/-\\|";
char *spin = "";

static inline void kvgacursoroff() {
	char *offset = vidmem+VGA_CALC_OFFSET(xpos,ypos);

	offset[1] = VGA_ATTR;
}


static inline  void kvgacursoron() {
	char *offset = vidmem+VGA_CALC_OFFSET(xpos,ypos);

	offset[1] = VGA_ATTR_CURSOR;
}

/* kvgascroll: scroll the framebuffer from the bottom line
 */
static inline void kvgascroll() {
	char blank[] = {' ',VGA_ATTR};

	/* Scroll: dependant  on current kmemcpy implementation */
	kmemcpy(vidmem+VGA_CALC_OFFSET(0,1), vidmem+VGA_CALC_OFFSET(0,2), VGA_CALC_OFFSET(0,VGA_LINES-2));

	/* Clear bottom line */
	kmemstamp(vidmem+VGA_CALC_OFFSET(0,VGA_LINES-1), blank, 2, VGA_COLS);
}

static inline void kvgaclear() {
	char blank[] = {' ',VGA_ATTR};
	kmemstamp(vidmem, blank, 2, VGA_LINES*VGA_COLS);
	xpos = ypos = 0;
}

/* klinewrap: wrap to the next line and scroll if needed
 */
static inline void klinewrap() {
	ypos++;
 	xpos = 0;
	if (ypos == VGA_LINES) {
		ypos = VGA_LINES-1;
		kvgascroll();
	}
}

void kvgahead(char *cs, int offset, int len) {
	char * vmem = vidmem;
	vmem += offset * 2;
	while (len-- && *cs) {
		*(vmem++) = *(cs++);
		*(vmem++) = VGA_HEAD_ATTR;
	}
}

/*
 * set up VGA driver at a specific base address
 *
 * NOT MANDATORY - Only if we need to write to VGA memory somewhere weird
 */
#ifndef ARCH_i386
void kvga_init(void *base_address) {
	vidmem = base_address;
}
#endif

void kvga_spin() {
	if (! *spin || spin > (spinner + 5)) spin = spinner;
	vidmem[VGA_CALC_OFFSET(79, 0)] = *(spin++);
}

/* This gets called a lot.
 * Try to have it as lightweight as possible.
 */
void vgaprint(char *message) {
	int offset;

	if (_INTERRUPTS_ENABLED()) {
		/* Only bother with the mutex when interrupts are enabled.
		 * Theoretically kvga should only be used for the initial kernel
		 * load anyhow (although everything is currently using it...),
		 * but the worst that should happen is some screen corruption.
		 */
		while (LOCKED_SEMAPHORE(vga_print_s)) 
			;
		//SPIN_WAIT_SEMAPHORE(vga_print_s);
		SPIN_YIELD_SEMAPHORE(vga_print_s);
	}
	kvgacursoroff();

	for (;;) {
	#if 0
		Assert(xpos < VGA_COLS);
		Assert(ypos < VGA_LINES);
	#endif

		switch (*message) {

		case 0:
			kvgacursoron();
			if (_INTERRUPTS_ENABLED()) { RELEASE_SEMAPHORE(vga_print_s); }
			return;

		case '\n':
			klinewrap();
			break;
#if 0
		case '':
			kvgaclear();
			break;
#endif
		case '\t':
			xpos += 8;
			xpos -= xpos  % 8;
			if (xpos >= VGA_COLS) 
				klinewrap();
			break;

		default:
			offset = VGA_CALC_OFFSET(xpos,ypos);

			vidmem[offset] = *message;
			vidmem[offset+1] = VGA_ATTR;

			xpos++;
			if (xpos == VGA_COLS)
				klinewrap();
			}
		++message;

	}
}


void vgaprint_cli(char *message) { vgaprint(message); }
#if 0
/* only for use with interrupts disabled */
void vgaprint_cli(char *message) {
	int delta = 0;

	if (vga_print_s == 0) delta = 1;
	vga_print_s += delta;
	vgaprint(message);
	vga_print_s -= delta;
	vga_print_s--;
}
#endif
