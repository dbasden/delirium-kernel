#include <delirium.h>
#include <cpu.h>
#include <kvga.h>

#define exit __builtin_exit

extern void * fake_framebuffer;
extern size_t fork();
size_t write(int, void *, size_t);

void dump_framebuffer(char *buf) {
	int x, y;

	for (y=0; y<VGA_LINES; y++)
	  	for (x=0; x<VGA_COLS; x++)
			write(1, buf+VGA_CALC_OFFSET(x, y), 1);
}

void kpanic_generic() {
	write(1, "Kernel panic\n",13);
	khalt();
}

void setup_first_task() {
}

void new_kthread(void *p) {
	if (! fork()) {
		p;
		exit(0);
	}
}


void khalt() {
	dump_framebuffer(fake_framebuffer);
	exit(0);
}

void setup_memory() {
}
void setup_interrupts() {
}
