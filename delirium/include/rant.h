#ifndef __RANT_H
#define __RANT_H

/*
 * Rants:  Send queued messages through a soapbox
 */

#include <delirium.h>
#include "soapbox.h"
#include "ipc.h"

typedef void(*rant_handler_t)(message_t);

/* Register as a listener to the soapbox
 * rants only have a single receiver at the moment.
 *
 * returns soapbox number if exists, or 0 if not found
 */
int supplicate(soapbox_id_t soapboxid, rant_handler_t handler);

/*
 * pre: there is a message in the queue
 */
message_t get_next_rant(size_t);

/*
 * process the next message in the queue for the current thread,
 * and call it's handler
 *
 * pre: there is a message in the queue
 */
void believe();

/*
 * queue a message to the supplicant
 * returns the soapboxid iff the message could not be queued.
 * Note: there is no guarantee that even if a message was queued
 * that it will be received correctly. 
 */
int rant(soapbox_id_t soapboxid, message_t message);

soapbox_id_t renounce(soapbox_id_t soapboxid);

void init_rant();

#endif
