/*
 * ktimer.c: simple coarse timer system for DeLiRiuM
 *
 *
 * send a message to a soapbox every n microseconds
 * currently only accurate to the INT 0 timer, which is only
 * running at around 1Hz
 *
 * Copyright (c)2008  David Basden <davidb-delirium@rcpt.to>
 */

#include <delirium.h>
#include <soapbox.h>
#include <ipc.h>
#include <rant.h>
#include <cpu.h>

#define _KTIMER_PRIVATE
#include "ktimer.h"
#undef _KTIMER_PRIVATE

/* I'm not concerned about this wrapping in around 584 thousand years. */
volatile u_int64_t useconds_since_boot;

struct ktimer ktimers[KTIMER_MAX_TIMERS];

struct ktimer * volatile unused_timers_head;
struct ktimer * volatile timer_pqueue_head; /* ascending by expiry */

/* How often the timer calls us, in microseconds
 *
 * If the timer updates less than every hour, this is going to be annoying
 */
static u_int32_t tick_update_useconds;

static inline struct ktimer * pop_timer(struct ktimer * volatile *stack_head) {
	/* threadsafe */
	struct ktimer *taken;
	do {
		taken = *stack_head;
		if (taken == NULL) return NULL;
	} while ((void *)cmpxchg(*stack_head, taken->next, taken) != taken);
	return taken;
}
static inline void push_timer(struct ktimer * volatile *stack_head, struct ktimer *ktp) {
	/* threadsafe */
	do { ktp->next = *stack_head;
	} while ((void *)cmpxchg(*stack_head, ktp, ktp->next) != ktp->next);
}

void ktimer_init(u_int32_t timer_tick_useconds) {
	int i;

	useconds_since_boot = 0;
	tick_update_useconds = timer_tick_useconds;

	unused_timers_head = NULL;
	for (i=0; i<KTIMER_MAX_TIMERS; ++i) 
		push_timer(&unused_timers_head, &(ktimers[i]));
}


/* Event handler called by the INT 0 timer with interrupts off
 *
 * This code should be as lightweight as possible
 */
void ktimer_on_timer_tick() {
	struct ktimer *ktp;

	useconds_since_boot += tick_update_useconds;

	while (timer_pqueue_head != NULL && timer_pqueue_head->expiry <= useconds_since_boot) {
		ktp = pop_timer(&timer_pqueue_head);

		if (ktp->expiry > 0) { /* If expiry is 0, we don't signal. see add_ktimer */
			/* Signal callback_soapbox <= ktp->signal */
			message_t msg;

			msg.type = signal;
			msg.m.signal = ktp->signal;
			rant(ktp->callback_soapbox, msg);

			if (ktp->shots) {
				if (! --(ktp->shots)) {
					/* n-shot timer ran out. return to unused timer stack and don't reinsert */
					push_timer(&unused_timers_head, ktp);
					continue;
				}
			}
		}

		/* Reset expiry and re-insert timer into queue */
		ktp->expiry = useconds_since_boot + ktp->interval;
		if (timer_pqueue_head == NULL) {
			timer_pqueue_head = ktp;
		} else if (timer_pqueue_head->expiry > ktp->expiry) {
			ktp->next = timer_pqueue_head;
			timer_pqueue_head = ktp;
		} else {
			struct ktimer *p = timer_pqueue_head;
			while (p->next != NULL &&  ktp->expiry > p->next->expiry)
				p = p->next;
			ktp->next = p->next;
			p->next = ktp;
		}
	}
}

/*
 * signal soapbox callback_soapbox with signal 'signal' every no more than every
 * update_usecs microseconds
 *
 * iff shots > 0, signal only 'shots' times, otherwise signal indefinately
 *
 * threadsafe.
 *
 * returns non-zero on success
 * returns 0 if the timer couldn't be added
 */
int add_ktimer(soapbox_id_t callback_soapbox, u_int32_t update_usecs, u_int32_t shots, u_int64_t signal) {
	struct ktimer *ktp;

	ktp = pop_timer(&unused_timers_head);

	if (ktp == NULL)
		return 0; /* Awww. No more timers */


	/* If we set an expiry of 0, next timer tick the ktimer will correctly set our 
	 * expiry from our interval, as of the next tick. As such, the first callback is
	 * actually time t is actually resolution > t > 2*resolution  where resolution is
	 * the period of the ktimer tick
	 *
	 * There is also the message queueing delay, plus waiting for the destination to
	 * get context and process the queue, so it's not like this is really that critical.
	 */
	ktp->expiry = 0;

	ktp->callback_soapbox = callback_soapbox;
	ktp->interval = update_usecs;
	ktp->shots = shots;
	ktp->signal = signal;

	push_timer(&timer_pqueue_head, ktp);

	return 1;
}

u_int64_t get_usec_since_boot() {
	return useconds_since_boot;
}
