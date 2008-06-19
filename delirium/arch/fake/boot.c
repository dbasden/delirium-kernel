/*
 * Bootstrap for user space delirium wrapper
 */
#include <fake/fake.h>
#include <delirium.h>
#include <kvga.h>
#include <cpu.h>
#include <multiboot.h>
#include <klib.h>

#define malloc __builtin_malloc

void *fake_framebuffer;

int main(int argc, char **argv) {

	fake_framebuffer = malloc(80*25*2);
	kmemset(fake_framebuffer, ' ', 80*25*2);
	kvga_init(fake_framebuffer);
	cmain(MULTIBOOT_BOOTLOADER_MAGIC, 0);
	khalt();
	return 0;
}
