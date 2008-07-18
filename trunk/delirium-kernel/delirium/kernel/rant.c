#include <delirium.h>
#include <assert.h>

#include "cpu.h"
#include "paging.h"
#include "soapbox.h"
#include "ipc.h"
#include "klib.h"
#include "rant.h"

#include "dlib/memherd.h"
#include "dlib/queue.h"

#if 0
#include "i386/task.h"
#endif
#include "multitask.h"

#define USING_MEMHERD_FOR_RANTS
#undef USING_POOLS_FOR_RANTS

typedef void(*rant_handler_t)(message_t);

#ifdef USING_POOLS_FOR_RANTS

#include "kpools.h"

/* queue_link_t is 2 * 32 bit words or 2 ** 3 bytes 
 */ 
#define QUEUE_LINK_T_EXP	3

/* message_t is 5 * 32 bit words (20 bytes) and will fit into 2 ** 5 (32) bytes */
#define MESSAGE_T_EXP	5

#undef QUEUE_HERDED_ADD
#define QUEUE_POOLED_ADD(_q, _item)	({queue_link_t *_i; \
		_i = kpool_alloc(QUEUE_LINK_T_EXP);_i->item=(_item); \
		QUEUE_ADDLINK(_q, _i);})

#undef QUEUE_HERDED_POP
#define QUEUE_POOLED_POP(_q)	({void *_r; void *_i; \
		_r=(_q)->head; _i=QUEUE_PEEK(_q); QUEUE_DELETE(_q); \
		kpool_free(QUEUE_LINK_T_EXP, _r); _i;})
#endif

#ifdef USING_MEMHERD_FOR_RANTS
static herd_t	rant_message_herd;
static herd_t	rant_queue_herd;
#endif

/*
 * Rants:  Send queued messages through a soapbox
 */

#define THREAD_QUEUE(_thread_id) (&(threads[(_thread_id)].rants))

/* Register as a listener to the soapbox
 * rants only have a single receiver at the moment.
 *
 * returns soapbox number if exists, or 0 if not found
 */
int supplicate(soapbox_id_t soapboxid, rant_handler_t handler) {
	int soapboxidx;

	if (! VALID_SOAPBOX_ID(soapboxid)) return 0;
	soapboxidx = SOAPBOX_INDEX(soapboxid);
	if (soapboxes[soapboxidx].id != soapboxid) return 0;

	/*  Make sure the refcount isn't inconsistent even if supplicate is called twice */
	if (soapboxes[soapboxidx].supplicant.thread_id != 0 && soapboxes[soapboxidx].supplicant.thread_id != -1
	    && soapboxes[soapboxidx].supplicant.handler) {
		/* Decrease old refcount */
		(threads[soapboxes[soapboxidx].supplicant.thread_id].refcount)--;

		/* Wake the old thread if it was sitting in the listening state to
		 * handle the case where refcount == 0 */
		wake_thread(soapboxes[SOAPBOX_INDEX(soapboxid)].supplicant.thread_id);
	}
	soapboxes[soapboxidx].supplicant.thread_id = running_thread_id;
	soapboxes[soapboxidx].supplicant.handler = handler;
	(threads[running_thread_id].refcount)++;

	return soapboxes[soapboxidx].id;
}

/* deregister a listener from a soapbox 
 * returns the soapboxid iff successfully de-registered.
 * otherwise, returns 0
 */
soapbox_id_t renounce(soapbox_id_t soapboxid) {
	int soapboxidx;
	supplicant_t * supplicant;
	
	if (! VALID_SOAPBOX_ID(soapboxid)) return 0;
	soapboxidx = SOAPBOX_INDEX(soapboxid);
	if (soapboxes[soapboxidx].id != soapboxid) return 0;
	supplicant = &(soapboxes[soapboxidx].supplicant);
	if (supplicant->handler == NULL) return 0;
	supplicant->handler = NULL;	
	(threads[supplicant->thread_id].refcount)--;;

	/* Wake the old thread if it was sitting in the listening state to
	 * handle the case where refcount == 0 */
	wake_thread(soapboxes[SOAPBOX_INDEX(soapboxid)].supplicant.thread_id);

	supplicant->thread_id = -1;

	return soapboxid;
}


/*
 * queue a message
 * pre: there is free space in the queue herd
 * pre: there is free space in the message herd
 * pre: the message has a valid destination
 */
static inline void queue_message(message_t message) {
	message_t *outmsg;
	size_t dest_thread_id;

	dest_thread_id = soapboxes[SOAPBOX_INDEX(message.destination)].supplicant.thread_id;

#ifdef USING_MEMHERD_FOR_RANTS
	outmsg = memherd_getBlock(&rant_message_herd);
	*outmsg = message;
	QUEUE_HERDED_ADD(&rant_queue_herd, THREAD_QUEUE(dest_thread_id), outmsg);
#endif
#ifdef USING_POOLS_FOR_RANTS
	outmsg = kpool_alloc(MESSAGE_T_EXP);
	*outmsg = message;
	QUEUE_POOLED_ADD(THREAD_QUEUE(dest_thread_id), outmsg);
#endif
}

/*
 * Get next rant for a thread.
 *
 * pre: thread exists
 * pre: there is a message in the queue
 */
message_t get_next_rant(size_t thread_id) {
	message_t msg;
	message_t *p;

#ifdef USING_MEMHERD_FOR_RANTS
	p = QUEUE_HERDED_POP(&rant_queue_herd, THREAD_QUEUE(thread_id));
	msg = *p;
	memherd_freeBlock(&rant_message_herd, p);
#endif
#ifdef USING_POOLS_FOR_RANTS
	p = QUEUE_POOLED_POP(THREAD_QUEUE(thread_id));
	msg = *p;
	kpool_free(MESSAGE_T_EXP, p);
#endif

	return msg;
}

/*
 * process the outstanding messages in the queue for the current thread
 */
void believe() {
	message_t msg;
	size_t destidx;

	while (!QUEUE_ISEMPTY(&(threads[running_thread_id].rants))) {
		msg = get_next_rant(running_thread_id);
		if (!VALID_SOAPBOX_ID(msg.destination)) continue;

		destidx = SOAPBOX_INDEX(msg.destination);

		if (soapboxes[destidx].id != msg.destination) continue;
		if (soapboxes[destidx].supplicant.thread_id != running_thread_id) continue;
		((rant_handler_t)(soapboxes[destidx].supplicant.handler))(msg);
	}
}

/*
 * queue a message to the supplicant
 * returns the soapboxid iff the message could not be queued.
 * Note: there is no guarantee that even if a message was queued
 * that it will be received correctly. 
 */
int rant(soapbox_id_t soapboxid, message_t message) {

	if (! VALID_SOAPBOX_ID(soapboxid))
		return 0;
#ifdef USING_MEMHERD_FOR_RANTS
	if (! ( (rant_queue_herd.free_blocks) &&
	      (rant_message_herd.free_blocks)) ) {
	      kprint("[rant: No free blocks to queue!]\n");
		return 0;
	}
#endif

	message.sender = running_thread_id; 
	message.destination = soapboxid;
	queue_message(message);
	wake_thread(soapboxes[SOAPBOX_INDEX(soapboxid)].supplicant.thread_id);

	return soapboxid;
}

void init_rant() {

#ifdef USING_POOLS_FOR_RANTS
	assert(sizeof(queue_link_t) <= (1<< QUEUE_LINK_T_EXP));
	assert(sizeof(message_t) <= (1<< MESSAGE_T_EXP));
#endif

#ifdef USING_MEMHERD_FOR_RANTS
	rant_queue_herd = new_memherd(kgetpage(), PAGE_SIZE, sizeof(queue_link_t));
	rant_message_herd = new_memherd(kgetpage(), PAGE_SIZE, sizeof(message_t));

	kprintf("(%u rants max)",
			(rant_message_herd.free_blocks < rant_queue_herd.free_blocks) ?
			rant_message_herd.free_blocks : rant_queue_herd.free_blocks);
#endif

}
