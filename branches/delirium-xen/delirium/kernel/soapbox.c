/*
 * soapboxes
 *
 * soapboxes are a contact point for message passing. Nothing more.
 * They don't require a specific type of message passing to happen using
 * them (for that look in ipc).
 *
 * A soapbox is registered by anyone who wants to create a point of contact,
 * either sender or receiver
 *
 * A soapbox has a name, and a number associated with it. The name is a
 * character string, and is only used for the initial lookup. The soapbox
 * number is a unique id for that soapbox. As much as possible, delirium
 * should avoid the unique id ever being issued again, even if the soapbox
 * is deregistered.
 *
 * IMPLEMENTATION:
 *
 * The soapbox number is 32 bits wide. The lower l bits are used for the current
 * slot in the soapbox array, while the upper bits are the least significant 
 * (32-l) bits of the thread id. This is to avoid if at all possible having
 * messages sent to the wrong soapbox entry; The soapbox entry can be looked
 * up in the array using an O(1) operation, using the lower l bits. The soapbox
 * number in the array can be checked against the supplied soapbox number, and 
 * the message discarded if they don't match.
 *
 * This checking does NOT protect against malicious hijacking of deregistered
 * soapboxes; It is suggested that if required this be implemented in userspace.
 *
 * TODO: Contemplate using a 64bit wide soapbox number and thread ID. Check
 * them using 32bit word operations so that it's not slow as anything.
 */
#include <delirium.h>
#include "dlib/map.h"
#include "cpu.h"
#include "paging.h"
#include "klib.h"
#include "soapbox.h"


Semaphore soapbox_mutex;
soapbox_t soapboxes[MAX_SOAPBOXEN+1];
map_t nametoboxnum;
int next_soapbox_index;


int soapbox_cmp(void *a, void *b) { return kstrcmp(a, b); }

/* 
 * Get the soapbox id, given a name 
 * Returns 0 if not found
 */
soapbox_id_t get_soapbox_from_name(char *name) {
	void *s;

	SPIN_WAIT_SEMAPHORE(soapbox_mutex); {
		s = map_get(&nametoboxnum, name);
	} RELEASE_SEMAPHORE(soapbox_mutex);

	return ((s == NULL) ? 0 : ((soapbox_id_t) s));
}

/*
 * register a new soapbox
 *
 * returns the soapbox id iff the soapbox was registered
 * returns 0 if name is already registered, or if no more soapboxes 
 * can be allocated
 */ 
soapbox_id_t get_new_soapbox(char *name) {
	int sbidx;
	soapbox_id_t soapbox_id;

	soapbox_id = 0;

	SPIN_WAIT_SEMAPHORE(soapbox_mutex); {
		if ((next_soapbox_index <= MAX_SOAPBOXEN) && \
		    (map_get(&nametoboxnum, name) == NULL)) {

			sbidx = next_soapbox_index++;
			soapbox_id = GET_SOAPBOX_ID(sbidx, running_thread_id);
			soapboxes[sbidx].id = soapbox_id;
			if (map_set(&nametoboxnum, name, (void *)soapbox_id) == NULL)
				soapbox_id = 0;
		}
	} RELEASE_SEMAPHORE(soapbox_mutex);

	return soapbox_id;
}


/*
 * manually write to the soapbox map
 *
 * returns 0 if the name is already registered, or any other error
 * otherwise returns the soapbox id
 */
soapbox_id_t set_soapbox_from_name(soapbox_id_t id, char *name) {
	soapbox_id_t retval;

	retval = 0;
	
	SPIN_WAIT_SEMAPHORE(soapbox_mutex); {
		if ((map_get(&nametoboxnum, name) == NULL) && 
		    (map_set(&nametoboxnum, name, (void *)id) != NULL))
			retval = id;
	} RELEASE_SEMAPHORE(soapbox_mutex);

	return retval;
}

void setup_soapbox() {
	soapbox_id_t i;

	INIT_SEMAPHORE(soapbox_mutex);

	nametoboxnum  = map_new(kgetpage(), PAGE_SIZE, soapbox_cmp);

	for (i=1; i<=MAX_SOAPBOXEN; i++)
		soapboxes[i].id = 0;

	next_soapbox_index = 1;
}
