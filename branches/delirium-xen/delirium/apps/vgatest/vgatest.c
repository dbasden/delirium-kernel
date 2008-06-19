#include <delirium.h>
#include <i386/io.h>
#include <paging.h>
#include "delibrium/delibrium.h"

#include "vga/vga.h"
#include "vga/vga_modes.h"
#include "img.h"

void vga_demo() {
	u_int32_t *gmem;

	SET_VGA_PAL(0,	255,255,255);
	SET_VGA_PAL(1,	0,0,0);

	gmem = (void *) 0xa0000;
	memcpy((void *)gmem, image_data, 320*200);
}

void dream() {
	vga_openwindow();
	vga_loadregs(&vgaregs_320x200x256);
	vga_demo();
}
