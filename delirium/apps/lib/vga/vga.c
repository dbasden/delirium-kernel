#include <delirium.h>
#include <paging.h>
#include <i386/io.h>
#include "delibrium/delibrium.h"

#include "vga.h"
#include "vga_modes.h"

void vga_saveregs(struct vga_regs *saved) {
	int i;

	/* 
	 * When we save the registers, make sure that colour mode
	 * IO addresses are in use and that the CPU can access memory
	 */
	saved->misc_output_reg = inb(VGA_MISC_OUT_REG) | 0x3;
	outb(VGA_MISC_OUT_REG, saved->misc_output_reg);

	for (i=0; i<5; i++) saved->sequencer[i] = READ_VGA_SEQ(i);
	for (i=0; i<25; i++) saved->crt_controller[i] = READ_VGA_CRTC(i);
	for (i=0; i<9; i++) saved->graphics_controller[i] = READ_VGA_GFX(i);
	for (i=0; i<21; i++) saved->attribute_controller[i] = READ_VGA_PAL(i);

	inb(VGA_CRTC_STATUS_REG);
	outb(VGA_PAL_REG, 0x20);
}


void vga_loadregs(struct vga_regs *r) {
	int i;

	outb(VGA_MISC_OUT_REG, r->misc_output_reg);

	for (i=0; i<5; i++) WRITE_VGA_SEQ(i, r->sequencer[i]);

	/* unlock CRTC registers */
	outb(VGA_CRTC_INDEX_REG, 0x03);
	outb(VGA_CRTC_DATA_REG, inb(VGA_CRTC_DATA_REG) | 0x80);
	outb(VGA_CRTC_INDEX_REG, 0x11);
	outb(VGA_CRTC_DATA_REG, inb(VGA_CRTC_DATA_REG) & ~0x80);

	for (i=0; i<25; i++) WRITE_VGA_CRTC(i, r->crt_controller[i]);
	for (i=0; i<9; i++) WRITE_VGA_GFX(i, r->graphics_controller[i]);
	for (i=0; i<21; i++) WRITE_VGA_PAL(i, r->attribute_controller[i]);

	inb(VGA_CRTC_STATUS_REG);
	outb(VGA_PAL_REG, 0x20);
}


/* Page in video memory window at 0xa0000 */
void vga_openwindow() {
	void *pd;
	size_t p;

	pd = get_current_page_dir();
	for (p = 0xa0000; p < 0xa0000 + (64*1024); p += PAGE_SIZE)
		add_to_pagedir(pd, (void *)p, (void *)p);
}
