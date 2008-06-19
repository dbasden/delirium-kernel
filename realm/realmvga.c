/*
 * Simple text VGA output for realm
 *
 * (c)2005 David Basden <davidb-delirium@rcpt.to>
 */

#include <delirium.h>
#include <libd.h>

#include "realm.h"

static void *vga_base_ptr;

#define VGAPOS(x, y)	((x) * 2 + (y) * 160)

/*
 * write some ASCII text starting at a certain offset to the
 * screen
 */
void realmvga_write(size_t x, size_t y, 
		    char *string, size_t length, 
		    size_t colour) {
	char *buf = vga_base_ptr + VGAPOS(x, y);

	memset(buf, colour & 0xff, length * 2);
	for (; length > 0; length--, buf +=2, string++)
		*buf = *string;
}

void realmvga_init(void *base_ptr) {

	if (base_ptr == NULL)
		base_ptr = 0xb8000;
}
