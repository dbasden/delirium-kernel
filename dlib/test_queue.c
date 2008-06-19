#include <stdio.h>
#include <assert.h>

#include "memherd.h"
#include "queue.h"


char memblock[1000];

int main() {
	herd_t h;
	struct queue q;
	char * fisha = "a";
	char * fishb = "b";
	char * fishc = "c";

	h = new_memherd(memblock, 1000, sizeof(queue_link_t));

	QUEUE_INIT(&q);
	assert(QUEUE_ISEMPTY(&q));
	assert(QUEUE_ISEMPTY(&q));
	QUEUE_HERDED_ADD(&h, &q, fishc);
	assert(!QUEUE_ISEMPTY(&q));
	assert(!QUEUE_ISEMPTY(&q));

	assert(q.head != NULL);
	assert(q.tail != NULL);
	assert(q.head == q.tail);


	assert(QUEUE_HERDED_POP(&h, &q) == fishc);
	assert(QUEUE_ISEMPTY(&q));

	QUEUE_HERDED_ADD(&h, &q, fishb);
	assert(!QUEUE_ISEMPTY(&q));
	QUEUE_HERDED_ADD(&h, &q, fisha);
	assert(!QUEUE_ISEMPTY(&q));
	QUEUE_HERDED_ADD(&h, &q, fisha);
	assert(!QUEUE_ISEMPTY(&q));
	QUEUE_HERDED_ADD(&h, &q, fisha);
	assert(!QUEUE_ISEMPTY(&q));
	assert(!QUEUE_ISEMPTY(&q));
	QUEUE_HERDED_ADD(&h, &q, fishc);
	assert(!QUEUE_ISEMPTY(&q));

	assert(QUEUE_HERDED_POP(&h, &q) == fishb);
	assert(!QUEUE_ISEMPTY(&q));
	assert(QUEUE_HERDED_POP(&h, &q) == fisha);
	assert(!QUEUE_ISEMPTY(&q));
	assert(QUEUE_HERDED_POP(&h, &q) == fisha);
	assert(!QUEUE_ISEMPTY(&q));
	assert(QUEUE_HERDED_POP(&h, &q) == fisha);
	assert(!QUEUE_ISEMPTY(&q));
	assert(QUEUE_HERDED_POP(&h, &q) == fishc);
	assert(QUEUE_ISEMPTY(&q));

	
	return 0;
}
