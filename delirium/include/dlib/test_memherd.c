#include <stdio.h>
#include <assert.h>

#include "memherd.h"

char mem[4096];

int main() {
	int i;
	herd_t mh;

	mh = new_memherd(mem, 100, 10);
	assert(mh.total_blocks == 9);
	assert(mh.free_blocks == 9);

	assert(memherd_getBlock(&mh) == mem);
	assert(memherd_getBlock(&mh) == mem + 10);
	assert(memherd_getBlock(&mh) == mem + 20);
	assert(memherd_getBlock(&mh) == mem + 30);
	assert(memherd_getBlock(&mh) == mem + 40);
	assert(memherd_getBlock(&mh) == mem + 50);
	assert(memherd_getBlock(&mh) == mem + 60);
	assert(memherd_getBlock(&mh) == mem + 70);
	assert(memherd_getBlock(&mh) == mem + 80);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	memherd_freeBlock(&mh, mem+40);
	assert(memherd_getBlock(&mh) == mem + 40);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	memherd_freeBlock(&mh, mem+50);
	memherd_freeBlock(&mh, mem+10);
	memherd_freeBlock(&mh, mem+20);
	assert(memherd_getBlock(&mh) == mem + 20);
	assert(memherd_getBlock(&mh) == mem + 10);
	assert(memherd_getBlock(&mh) == mem + 50);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);

	memherd_freeBlock(&mh, mem);
	memherd_freeBlock(&mh, mem+10);
	memherd_freeBlock(&mh, mem+20);
	memherd_freeBlock(&mh, mem+30);
	memherd_freeBlock(&mh, mem+40);
	memherd_freeBlock(&mh, mem+50);
	memherd_freeBlock(&mh, mem+60);
	memherd_freeBlock(&mh, mem+70);
	memherd_freeBlock(&mh, mem+80);
	assert(mh.free_blocks == 9);

	assert(memherd_getBlock(&mh) == mem + 80);
	assert(memherd_getBlock(&mh) == mem);
	assert(memherd_getBlock(&mh) == mem + 10);
	assert(memherd_getBlock(&mh) == mem + 20);
	assert(memherd_getBlock(&mh) == mem + 30);
	assert(memherd_getBlock(&mh) == mem + 40);
	assert(memherd_getBlock(&mh) == mem + 50);
	assert(memherd_getBlock(&mh) == mem + 60);
	assert(memherd_getBlock(&mh) == mem + 70);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	memherd_freeBlock(&mh, mem+40);
	assert(memherd_getBlock(&mh) == mem + 40);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	memherd_freeBlock(&mh, mem+50);
	memherd_freeBlock(&mh, mem+10);
	memherd_freeBlock(&mh, mem+20);
	assert(memherd_getBlock(&mh) == mem + 20);
	assert(memherd_getBlock(&mh) == mem + 10);
	assert(memherd_getBlock(&mh) == mem + 50);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);

	memherd_freeBlock(&mh, mem);
	memherd_freeBlock(&mh, mem+10);
	memherd_freeBlock(&mh, mem+20);

	mh = new_memherd(mem, 4096, 10);
	assert(mh.free_blocks == 404);
	for (i =0; i<404; i++)
		assert(memherd_getBlock(&mh) == mem + (i*10));

	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	memherd_freeBlock(&mh, mem+4000);
	memherd_freeBlock(&mh, mem+400);
	memherd_freeBlock(&mh, mem+1200);
	memherd_freeBlock(&mh, mem+2800);
	assert(memherd_getBlock(&mh) == mem + 2800);
	assert(memherd_getBlock(&mh) == mem + 4000);
	assert(memherd_getBlock(&mh) == mem + 400);
	assert(memherd_getBlock(&mh) == mem + 1200);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);
	assert(memherd_getBlock(&mh) == NULL);

	return 0;
}
