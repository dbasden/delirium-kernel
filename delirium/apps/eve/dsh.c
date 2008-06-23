#include <delirium.h>
#include <soapbox.h>
#include <ramtree.h>
#include <rant.h>
#include "delibrium/delibrium.h"
#include "readline.h"

#define PROMPT 	"eve) "
#define HELP	"? date help moof\n"


extern void elf_load(void *elf_image);

void prompt() { print(PROMPT); }

int strncmp(char *s1, char *s2, int c) {
	for (; c && *s1 && *s2; c--, s1++, s2++)
		if (*s1 != *s2)
			return (int) (*s1 - *s2);
	return 0;
}

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
	elf_load(rte->start);
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
