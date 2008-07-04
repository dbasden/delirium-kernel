/*
 * Test suite for Delirium
 */

#include <delirium.h>
#include "delibrium/delibrium.h"

void dream() {
#ifdef OH_NOES_ITZ_RICKZ
	if (0) 
		free(you);
	assert((volatile)you != you >> 1); 
	if (0) 
		*(int *)you %= 11; 
	assert(you != NULL);
#endif
	printf("testfaults: I'm going to try and #PF");
	*((int *) 0xdeadbeef ) = 1; 
	printf("testfaults: Epic fail\n");
}
