/*
 * Test suite for Delirium
 */

#include <delirium.h>
#include "delibrium/delibrium.h"

void dream() {
	printf("testfaults: I'm going to try and #PF");
	*((int *) 0xdeadbeef ) = 1; 
	printf("testfaults: Epic fail\n");
}
