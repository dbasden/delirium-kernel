#ifndef __CQUEUE_H
#define __CQUEUE_H

#include <delirium.h>
/*
 * Cyclic block queue
 */

struct cbQueue {
	char* cq;
	size_t head;
	size_t tail;
	size_t len;
};
typedef struct cbQueue *	cbQueue_t;
#define cyclicIncBy(_q, _i, _n) ( ((_i) + (_n)) % ((_q)->len) )
#define cyclicDecBy(_q, _i, _n) ( ((_i)+((_q)->len)-(_n)) % ((_q)->len) )

/*
 * Next index from _i in a queue
 */
#define cyclicInc(_q, _i) cyclicIncBy(_q, _i, 1)

/*
 * Previous index from _i in a queue
 */
#define cyclicDec(_q, _i) cyclicDecBy(_q, _i, 1)

/*
 * Setup a cbQueue
 *
 * _q		pointer to struct cbQueue
 * _array	array to set cq to
 */
#define cyclicInit(_q, _array, _len) (\
		(_q)->cq = (_array), \
		(_q)->len = (_len), \
		(_q)->head = (_q)->tail = 0 )

#define cbQinit cyclicInit
#define cbQincHead(_q, _n)  ( (_q)->head = cyclicIncBy(_q, (_q)->head, _n) ) 
#define cbQincTail(_q, _n)  ( (_q)->tail = cyclicIncBy(_q, (_q)->tail, _n) )

#define cbQpopN(_q, _n) cbQincHead(_q, _n)

#define cbQisEmpty(_q) ( (_q)->head == (_q)->tail )
#define cbQisFull(_q) ( (_q)->tail == cyclicDec(_q, (_q)->head) )

#define cbQadd(_q, _item) ( ((_q)->cq[(_q)->tail] = _item) , cbQincTail(_q, 1) )
#define cbQpop(_q) ( cbQpopN(_q, 1), (_q)->cq[ cyclicDec(_q, (_q)->head) ] )

#endif
