#ifndef _KVGA_H
#define _KVGA_H

void vgaprint(char *message);
void vgaprint_cli(char *message);	/* not for general use */
void kvga_init(void *framebuffer);
void kvga_spin();

/* Base memory location of the text buffer */
#define VGA_BASE	((void *)0xb8000)

/* Easily calculate offset into vga memory */
#define VGA_CALC_OFFSET(x,y)    (((x) + ((y) * VGA_COLS)) * 2)

/* Columns and lines */
#define VGA_COLS	80
#define	VGA_LINES	25

/* Attribue to use for kernel writing */
#define VGA_ATTR	7
#define VGA_ATTR_CURSOR	112

#endif
