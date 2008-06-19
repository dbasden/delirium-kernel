#include <delirium.h>
#include <assert.h>
#include <ipc.h>

#include "paging.h"
#include "dlib/cqueue.h"

/*
 * Here we define the linear and gestalt message types (with apologies 
 * to Greg Egan)
 *
 * Linear messages are for ordered data. Gestalt messages are for coherent
 * blocks of data. Obviously the distinction is purely arbitary, and the
 * choice of whichever is appropriate is left up the the user. There is
 * no reason not to convert between the two.
 */

/*
 * Create new linear, 
 */
linear_t linear_new() {
	void *page;
	linear_t lin;

	page = kgetpage();
	lin = LINEAR_STRUCT_POS(page);
	lin->page = page;
	cbQinit(&(lin->q), (char *) lin->page, LINEAR_BUFSIZE);

	return lin;
}

/* Return spare space in linear buffer */
size_t linear_spare(linear_t l) {
	if ((l->q).head > (l->q).tail)
		return (l->q).len - 
			(l->q.head - l->q.tail);
	return (l->q).len - l->q.tail + l->q.head;
}


/*
 * get a pointer to the next writable part of
 * the linear. max is the maximum that will be committed
 * with linear_commit. If max is > linear_spare,
 * the difference will be release in the queue.
 *
 * NULL is returned iff max > linear_size()
 */
void * linear_getwp(linear_t l, size_t max) {
	if (max > linear_spare(l)) {
		if (max > linear_size(l))
			return NULL;
		linear_release(l, max - linear_spare(l));
	}
	return l->q.cq + l->q.tail;
}

