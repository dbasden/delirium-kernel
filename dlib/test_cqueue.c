#include <stdio.h>
#include <assert.h>
#include "cqueue.h"
#if 0
#define cbQincHead(_q)  ( (_q)->head = cyclicInc(_q, (_q)->head) ) 
#define cbQincTail(_q)  ( (_q)->tail = cyclicInc(_q, (_q)->tail) )

#define cbQisEmpty(_q) ( (_q)->head == (_q)->tail )
#define cbQisFull(_q) ( (_q)->tail == cyclicDec(_q, (_q)->head) )

#define cbQadd(_q, _item) ( ((_q)->cq[(_q)->tail]) , cbQincTail(_q) )
#define cbQpop(_q) ( cbQincHead(_q), (_q)->cq[ cyclicDec(_q, (_q)->head) ] )
#endif

int main() {
	char fishy[256];
	struct cbQueue cbq;
	cbQueue_t myQueue = &cbq;
	char c;

	cbQinit(myQueue, fishy, 1);
	assert(cbQisEmpty(myQueue));
	assert(cbQisFull(myQueue));

	cbQinit(myQueue, fishy, 4);
	assert(cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	cbQadd(myQueue, 'a');
	assert(!cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	c = cbQpop(myQueue);
	assert(c == 'a');
	assert(cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	cbQadd(myQueue, 'c');
	assert(!cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	cbQadd(myQueue, 'd');
	assert(!cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	c = cbQpop(myQueue);
	assert(c == 'c');
	assert(!cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	cbQadd(myQueue, 'e');
	assert(!cbQisEmpty(myQueue));
	assert(!cbQisFull(myQueue));

	cbQadd(myQueue, 'f');
	assert(!cbQisEmpty(myQueue));
	assert(cbQisFull(myQueue));

	c = cbQpop(myQueue); assert(c == 'd'); assert(!cbQisEmpty(myQueue)); assert(!cbQisFull(myQueue));
	c = cbQpop(myQueue); assert(c == 'e'); assert(!cbQisEmpty(myQueue)); assert(!cbQisFull(myQueue));
	c = cbQpop(myQueue); assert(c == 'f'); assert(cbQisEmpty(myQueue)); assert(!cbQisFull(myQueue));

	return 0;
}
