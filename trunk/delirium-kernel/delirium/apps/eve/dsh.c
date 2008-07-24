#include <delirium.h>
#include <soapbox.h>
#include <ramtree.h>
#include <rant.h>
#include "delibrium/delibrium.h"
#include "readline.h"
#ifdef ARCH_i386
#include <i386/io.h>
#endif

#define PROMPT 	"eve) "
#define HELP	"? date help moof run signal inb outb inw outw inl outl\n"


extern void eve_elf_load(void *elf_image);

void prompt() { print(PROMPT); }

#if 0
/* Now in delibrium*/
int strncmp(char *s1, char *s2, int c) {
	for (; c && *s1 && *s2; c--, s1++, s2++)
		if (*s1 != *s2)
			return (int) (*s1 - *s2);
	return 0;
}
#endif

// returns num of characters in the next word
static int next_word(char * line, char token) {
	char *w = line;
	while (*w && (*w != token)) w++;
	return w-line;
}

static void send_signal(char *name, int signal_number) {
	soapbox_id_t soapbox;
	message_t outmsg;

	soapbox = get_soapbox_from_name(name);
	if (soapbox == 0) {
		printf("Couldn't find soapbox %s to signal.\n", name);
		return;
	}

	outmsg.type = signal;
	outmsg.m.signal = signal_number;
	rant(soapbox, outmsg);
}

static void do_exec(char *name) {
	struct ramtree_entry * rte;

	rte =  (struct ramtree_entry *) get_soapbox_from_name(name);
	if (rte == NULL) {
		printf("Couldn't find ramtree entry for '%s'\n", name);
		return;
	}
	printf("Found %s at 0x%8x\n", rte->name, rte->start);
	eve_elf_load(rte->start);
}

static void break_tests() {
	print("eve break_tests(): Dumping initial debug state\n");
	send_signal("kdebug/soapbox", 0);
	send_signal("kdebug/thread", 0);
	print("eve break_tests(): Starting a few test instances\n");
	int i;
	for (i=0; i<10;++i)	
		 { do_exec("apps/test"); send_signal("kdebug/thread", 0); }
	send_signal("kdebug/soapbox", 0);
	send_signal("kdebug/thread", 0);
	print("eve break_tests(): Done\n");
}


void dsh_parse(char *l) {
	int wl;

	if (!(wl = next_word(l, ' '))) return;

	if (*l == '?')  {
		print(HELP);
	} else
	if (!strncmp("run", l, wl)) {
		l = l + wl + 1;
		if (!(wl = next_word(l, ' '))) {
			print("You run like a little fish.\n");
			return;
		}
		printf("running %s\n", l);
		do_exec(l);
		return;
	} else
	if (!strncmp("signal", l, wl)) {
		int signum;

		l = l + wl + 1;
		if (!(wl = next_word(l, ' '))) {
			print("usage: signal <number> <soapbox name...\n");
			return;
		}
		signum = atoi(l);
		l = l + wl + 1;
		if (!(wl = next_word(l, ' '))) {
			print("usage: signal <number> <soapbox name...\n");
			return;
		}
		send_signal(l, signum);
	} else
	if (!strncmp("help", l, wl)) {
		print("You are beyond help\n");
	} else
	if (!strncmp("moof", l, wl)) {
		print("Dogcow\n");
	} else 
	if (wl && l[0] == '!') {
		print("eve: Running break-tests...\n");
		break_tests();
	} else
	if (!strncmp("date", l, wl)) {
		print("It is the 32nd of Borktober!\n");
	} else 
	if (!strncmp("yes", l, wl)) {
		print("So that's some fries.\nWould you like fries with that?\n");
#ifdef ARCH_i386
	} else 
	if (!strncmp("inb", l, wl)) {
		int port; int byte;
		l = l + wl + 1; 
		if (!(wl = next_word(l, ' '))) {
			print("usage: inb <decimal ioport>\n"); return;
		}
		port = atoi(l); byte = inb(port);
		printf("io port 0x%4x = 0x%x\n", port, byte);
	} else
	if (!strncmp("inw", l, wl)) {
		int port; int word;
		l = l + wl + 1; 
		if (!(wl = next_word(l, ' '))) {
			print("usage: inw <decimal ioport>\n"); return;
		}
		port = atoi(l); word = inb(port);
		printf("io port 0x%4x = 0x%x\n", port, word);
	} else
	if (!strncmp("outb", l, wl)) {
		int port; int byte;
		l = l + wl + 1; if (!(wl = next_word(l, ' '))) {
			print("usage: outb <decimal ioport> <decimal byte>\n"); return;
		}
		port = atoi(l);
		l = l + wl + 1; if (!(wl = next_word(l, ' '))) { return; }
		byte = atoi(l); outb(port, byte);
		printf("outb: io port 0x%4x <-- 0x%x\n", port, byte);
	} else 
	if (!strncmp("outw", l, wl)) {
		int port; int word;
		l = l + wl + 1; if (!(wl = next_word(l, ' '))) {
			print("usage: outw <decimal ioport> <decimal uint16_t>\n"); return;
		}
		port = atoi(l);
		l = l + wl + 1; if (!(wl = next_word(l, ' '))) { return; }
		word = atoi(l); outw(port, word);
		printf("outw: io port 0x%4x <-- 0x%x\n", port, word);
#endif
	} else {
		printf("So thats a %s.\nWould you like fries with that?\n", l);
	} 
}

void dsh_line(char *l) {
	dsh_parse(l);
	prompt();
}

void start_dsh() {
	initReadline(&dsh_line);
	prompt();
}
