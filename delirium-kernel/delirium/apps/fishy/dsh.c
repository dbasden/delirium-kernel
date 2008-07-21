#include <delirium.h>
#include "delibrium/delibrium.h"
#include "readline.h"

#define PROMPT 	"dsh) "
#define HELP	"? date help life moof\n" 

void prompt() { printf(PROMPT); }

#if 0
int strncmp(char *s1, char *s2, int c) {
	for (; c && *s1 && *s2; c--, s1++, s2++)
		if (*s1 != *s2)
			return (int) (*s1 - *s2);
	return *s1 - *s2;
}
#endif

// returns num of characters in the next word
static int next_word(char * line, char token) {
	char *w = line;
	while (*w && *w != token) w++;
	return w-line;
}


void dsh_parse(char *l) {
	int wl;

	if (!(wl = next_word(l, ' '))) return;

	if (*l == '?')  {
		print(HELP);
	} else
	if (!strncmp("help", l, wl)) {
		print("You are beyond help\n");
	} else
	if (!strncmp("moof", l, wl)) {
		print("Dogcow\n");
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
