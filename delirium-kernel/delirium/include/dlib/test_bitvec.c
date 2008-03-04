#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

#include "bitvec.h"

int main() {
	bitvec_t bv;
	int i;

	bv = calloc(BITVEC_SIZE(2000),1);

	assert(BITVEC_SIZE(2016) == 252);
	assert(BITVEC_SIZE(2017) == 256);

	for (i=0; i<2000; i++)
		assert(! BITVEC_GET(bv,i));

	BITVEC_SET(bv, 1532);

	assert(BITVEC_GET(bv,1532));

	for (i=0; i<1532; i++)
		assert(! BITVEC_GET(bv,i));

	for (i=1533; i<2000; i++)
		assert(! BITVEC_GET(bv,i));

	free(bv);

	return 0;
}
