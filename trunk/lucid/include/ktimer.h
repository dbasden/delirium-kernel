#ifndef __KTIMER_H
#define __KTIMER_H

/* simple coarse timer system
 *
 * send a message to a soapbox every n microseconds
 *
 * currently only accurate to the INT 0 timer, which is only
 * running at around 1Hz
 */

#include <delirium.h>
#include <soapbox.h>

#ifdef _KTIMER_PRIVATE

#define KTIMER_MAX_TIMERS	1000
/* I'm not concerned about this wrapping in around 584 thousand years. */
extern volatile u_int64_t useconds_since_boot;

struct ktimer {
	struct ktimer *	next;
	soapbox_id_t 		callback_soapbox;
	u_int64_t		expiry; /* Next timer expiry */
	u_int32_t		interval; /* How often to update in useconds */
	u_int32_t		shots;	/* If this is an n-shot timer, how many shots left, otherwise 0 */
	u_int64_t		signal;
};

extern struct ktimer ktimers[KTIMER_MAX_TIMERS];

extern struct ktimer * volatile unused_timers_head;
extern struct ktimer * volatile timer_pqueue_head; /* ascending by expiry */

#endif

/* Setup the ktimer.
 * timer_tick_useconds is how often the timer will call ktimer_on_timer_tick()
 */
void ktimer_init(u_int32_t timer_tick_useconds);

/* Event handler called by the INT 0 timer with interrupts off
 */
void ktimer_on_timer_tick();

/*
 * signal soapbox callback_soapbox with signal 'signal' every update_usecs microseconds
 *
 * iff shots > 0, signal only 'shots' times, otherwise signal indefinately
 *
 * returns non-zero on success
 * returns 0 if the timer couldn't be added
 */
int add_ktimer(soapbox_id_t callback_soapbox, u_int32_t update_usecs, u_int32_t shots, u_int64_t signal);

u_int64_t get_usec_since_boot();

#endif
