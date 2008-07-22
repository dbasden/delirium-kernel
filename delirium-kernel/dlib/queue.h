#ifndef __QUEUE_H
#define __QUEUE_H

struct list_link {
	struct list_link *next;
	void *item;
};

typedef struct list_link queue_link_t;
struct queue {
	queue_link_t *head;
	queue_link_t *tail;
};

#define QUEUE_ISEMPTY(_q) ((_q)->head == NULL)
#define QUEUE_INIT(_q) ((_q)->head = (_q)->tail = NULL)
#define QUEUE_ADDLINK(_q, _link) ( ((_link)->next = NULL), \
		(	(QUEUE_ISEMPTY(_q)) ? \
			( (_q)->head = (_link) ) : \
			( (_q)->tail->next = (_link) ) \
		), ((_q)->tail = (_link)) )
#define QUEUE_PEEK(_q) ((_q)->head->item) 
#define QUEUE_DELETE(_q) ( ((_q)->head = (_q)->head->next), (_q)->head == NULL ? (_q)->tail = NULL : (_q)->head )

/* For use with the herd stuff */
#define QUEUE_HERDED_ADD(_herd, _q, _item)	({queue_link_t *_i; \
		_i = memherd_getBlock(_herd);_i->item=(_item); \
		QUEUE_ADDLINK(_q, _i);})
#define QUEUE_HERDED_POP(_herd, _q)	({void *_r; void *_i; \
		_r=(_q)->head; _i=QUEUE_PEEK(_q); QUEUE_DELETE(_q); \
		memherd_freeBlock(_herd, _r); _i;})
#endif

