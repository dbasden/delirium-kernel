#include <delirium.h>
#include <assert.h>
#include <cpu.h>

#ifdef ARCH_i386
#include "i386/task.h"
#endif
#include "ipc.h"
#include "rant.h"
#include "soapbox.h"
#include "multitask.h"


void debug_thread_listener(message_t msg) {
	int i;
	int thread_count = 0;
	const char *run_str = "running";
	const char *listen_str = "listening";
	const char *idle_str = "(idle)";

	if (msg.type != signal) return;

	kprintf("\n| [ Threads ]\n");
#ifdef ARCH_i386
	kprintf("| id\tstate\t\trefcount\thasrants\tcontext switches\n| --\t-----\t\t--------\t--------\t------------\n");
	for (i=0; i<MAX_THREADS;i++) {
		const char *state_str;

		if (threads[i].state == dead) continue;
		++thread_count;
		switch(threads[i].state) {
		case running: state_str = run_str; break;
		case listening: state_str = listen_str; break;
		case idle: state_str = idle_str; break;
		default: state_str = "Unknown";
		}

		kprintf("| %d\t%s\t\t%d\t%s\t%d\n", i, state_str,
				threads[i].refcount,
				QUEUE_ISEMPTY(&(threads[i].rants)) ? "no" : "yes",
				threads[i].context_switches);
	}
	kprintf("\n\n| %d threads total\n| %d context switches since boot\n", 
			thread_count, global_context_switches);
#endif
}

void debug_soapbox_listener(message_t msg) {
	int i;
	if (msg.type != signal) return;

	kprintf(" SOAPBOXES:\n| index\tid\t\tthread hint\tsupplicant\n");
	for (i=0; i<MAX_SOAPBOXEN; i++) {
		if (soapboxes[i].id == 0) continue;

		kprintf("\n| %d\t0x%8x\t%d", i, soapboxes[i].id,
				SOAPBOX_THREAD_HINT(soapboxes[i].id));
		if (soapboxes[i].supplicant.handler != NULL) {
			kprintf("\tthread #%d. handler 0x%8x", 
				soapboxes[i].supplicant.thread_id, soapboxes[i].supplicant.handler);
		}
			
	}
	kprintf("\n");
}

void debug_task() {
	soapbox_id_t thread_soapbox;
	soapbox_id_t soapbox_soapbox;
	soapbox_id_t ret;

	thread_soapbox = get_new_soapbox("kdebug/thread");
	ret = supplicate(thread_soapbox, debug_thread_listener);
	Assert(ret);

	soapbox_soapbox = get_new_soapbox("kdebug/soapbox");
	ret = supplicate(soapbox_soapbox, debug_soapbox_listener);
	Assert(ret);

	for (;;) {
		believe();
		await();
	}
}

void setup_debug_listener() {
	new_kthread(debug_task);
}

