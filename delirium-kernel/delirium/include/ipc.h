#ifndef __IPC_H
#define __IPC_H

#include "dlib/cqueue.h"

/* Different types of messages */
enum message_type {linear, gestalt, signal};
typedef enum message_type message_type_t;

/*
 * Message definitions
 */
#if 0
struct _gestalt {
	void *		first_page;
	size_t		pages;
};
typedef struct _gestalt * gestalt_t;
#endif

#if 0
typedef struct _gestalt * gestalt_t;

struct _signal {
	size_t		widget;
};
typedef struct _signal * signal_t;
#endif

typedef void * gestalt_t;
typedef u_int32_t signal_t;

struct _linear {
	void *		page;
	struct cbQueue	q;
};
typedef struct _linear * linear_t;


/*
 * A 'Generic' to wrap the messages inside of
 * so they can be passed without caring about the
 * type
 */
struct _message {
	message_type_t 	type;
	size_t		sender;
	size_t		destination;
	union {
		gestalt_t gestault;
		linear_t linear;
		signal_t signal;
	} m;
};
typedef struct _message message_t;


/*
 * LINEAR
 */

/* In this case, must be the same as the page size */
#define LINEAR_BUFSIZE	(4096 - sizeof(struct _linear))

#define LINEAR_STRUCT_POS(_base) ((linear_t)((_base) + LINEAR_BUFSIZE))

/* get a new linear */
linear_t linear_new();

/* Destroy and free the linear */
#define linear_free(_l)	(kfreepage(l->page))

/* return the maximum space allocated to the linear buffer */
#define linear_size(_l) ((_l)->q.len - 1)

/* Return spare space in linear buffer */
size_t linear_spare(linear_t l);

/* return the bytes of data buffered in the linear */
#define linear_used(_l) (linear_size(_l) - linear_spare(_l))

/* 
 * release n bytes from the beginning of the linear buffer,
 * to be able to be used again for buffer space
 *
 * pre: n <= linear_used()
 */
#define linear_release(_l, _n) ((_n) <= linear_used(_l)), cbQpopN(&(_l->q), _n) 
#if 0
#define linear_release(_l, _n) (Assert((_n) <= linear_used(_l)),\
		cbQpopN(&(_l->q), _n) )
#endif

/*
 * get a pointer to the next writable part of
 * the linear. max is the maximum that will be committed
 * with linear_commit. If max is > linear_spare,
 * the difference will be release in the queue.
 *
 * NULL is returned iff max > linear_size()
 */
void * linear_getwp(linear_t l, size_t max);

/*
 * commit bytes written to the linear
 *
 * linear_getwp gives you a pointer to the space free in
 * the linear, releasing the space if needed. The bytes
 * written to that buffer aren't part of the linear until
 * committed.
 *
 * TODO: put a spinlock on getwp, and release it here
 */
#define linear_commit(_l, _bytes) (Assert((_bytes) <= linear_spare(_l))\
		, cbQincTail(&((_l)->q), _bytes))

/*
 * Push a single byte onto the linear. Doesn't require
 * linear_getwp or linear_commit
 *
 * pre: linear_spare >= 1
 */
#define linear_addb(_l, _byte)	cbQadd(&((_l)->q), _byte)

#endif
