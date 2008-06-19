#include <delirium.h>
#include <ipc.h>
#include <rant.h>

#include "delibrium/delibrium.h"
#include "readline.h"

extern void start_dsh();

void get_kb_message(message_t msg) {
	printf("%c", msg.m.signal);
	keyInput(msg.m.signal);
}

void dream() {
	soapbox_id_t	keyboard_soapid;

	printf("We are the music makers. And we are the dreamers of the dream.");

	while (!( keyboard_soapid = get_soapbox_from_name("/hardware/keyboard")))
			yield();

	start_dsh();

	if (!supplicate(keyboard_soapid, get_kb_message)) {
		printf("fishy: Couldn't supplicate keyboard soapbox. Exiting\n");
		return;
	}

	while (1) {
		believe();
		yield();
	}
}
