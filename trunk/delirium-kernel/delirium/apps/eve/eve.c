#include <delirium.h>
#include <ipc.h>
#include <rant.h>
#include <multitask.h>

#include "delibrium/delibrium.h"
#include "readline.h"

extern void start_dsh();

void get_kb_message(message_t msg) {
	printf("%c", msg.m.signal);
	keyInput(msg.m.signal);
}

void dream() {
	soapbox_id_t	keyboard_soapid;

	while (!( keyboard_soapid = get_soapbox_from_name("/hardware/keyboard")))
			yield();

	start_dsh();

	if (!supplicate(keyboard_soapid, get_kb_message)) {
		printf("eve: Couldn't supplicate keyboard soapbox. Exiting\n");
		return;
	}

	// TODO: Make this the default behaviour
	for (;;) {
		believe();
		await();
	}
}
