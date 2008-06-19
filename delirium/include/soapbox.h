#ifndef __SOAPBOX_H
#define __SOAPBOX_H

#include <delirium.h>

typedef size_t soapbox_id_t;

struct supplicant {
	size_t	thread_id;
	void	* handler;
};
typedef struct supplicant supplicant_t;

struct soapbox {
        soapbox_id_t id;
	supplicant_t supplicant;
};
typedef struct soapbox soapbox_t;

#define SOAPBOX_INDEX_BITS      16

/* Arbitary size. Cant be biger than the soapbox index bitfield */
#define MAX_SOAPBOXEN   1024

/* Index into the soapbox array */
#define SOAPBOX_INDEX_MASK ((1 << SOAPBOX_INDEX_BITS)-1)
#define SOAPBOX_INDEX(_soapboxid) ((_soapboxid) & SOAPBOX_INDEX_MASK)

/* Get the lower bits of the thread number given a soapbox id*/
#define SOAPBOX_THREAD_HINT(_soapboxid) ((_soapboxid) >> SOAPBOX_INDEX_BITS)

/* Create a soapbox id given the a soapbox index and a thread id */
#define GET_SOAPBOX_ID(_index, _threadid) (((_threadid)<<SOAPBOX_INDEX_BITS)| \
	                                ((_index) & (SOAPBOX_INDEX_MASK)) )

#define VALID_SOAPBOX_ID(_id) ( (SOAPBOX_INDEX(_id) <= MAX_SOAPBOXEN) && \
		        (soapboxes[SOAPBOX_INDEX(_id)].id == (_id)) )


extern soapbox_t soapboxes[];
void setup_soapbox();
soapbox_id_t get_soapbox_from_name(char *name);
soapbox_id_t get_new_soapbox(char *name);
soapbox_id_t set_soapbox_from_name(soapbox_id_t id, char *name);

#endif
