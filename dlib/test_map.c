#include <stdio.h>
#include <assert.h>

#include "map.h"

int mystrcmp(void *a, void *b) {
	char *as = a;
	char *bs = b;

	while ((*as != 0) && (*bs != 0) && (*as == *bs)) {
		as++;
		bs++;
	}

	return *as - *bs;
}

char mem[4096];

int main() {
	map_t map;
	map_t *m;

	assert(mystrcmp("agfdg", "sadf") < 0);
	assert(mystrcmp("zgfdg", "sadf") > 0);
	assert(mystrcmp("z", "sadf") > 0);
	assert(mystrcmp("za", "sadf") > 0);
	assert(mystrcmp("za", "zadf") < 0);
	assert(mystrcmp("", "zadf") < 0);
	assert(mystrcmp("za", "") > 0);
	assert(mystrcmp("", "") == 0);
	assert(mystrcmp("a", "a") == 0);
	assert(mystrcmp("ab", "ab") == 0);
	assert(mystrcmp("ac", "ab") > 0);

	map = map_new(mem, 4096, mystrcmp);
	m = &map;

	assert(map_spareEntries(m) == 170);
	assert(map_get(m, "fish") == NULL);
	assert(map_get(m, "heads") == NULL);
	assert(map_get(m, "") == NULL);

	assert(!mystrcmp(map_set(m, "fish", "wibble"),"wibble"));
	assert(!mystrcmp(map_set(m, "heads", ""),""));
	assert(!mystrcmp(map_set(m, "fish", "gimps"),"gimps"));
	assert(!mystrcmp(map_set(m, "fish", "gimps"),"gimps"));
	assert(!mystrcmp(map_set(m, "fish", "gimps"),"gimps"));

	assert(map_spareEntries(m) == 168);
	assert(mystrcmp(map_get(m, "fish"),"wibble") < 0);
	assert(!mystrcmp(map_get(m, "heads"),""));
	assert(!mystrcmp(map_get(m, "fish"),"gimps"));
	assert(map_get(m, "") == NULL);
	assert(map_get(m, "wibble") == NULL);
	assert(map_get(m, "gimps") == NULL);
	map_unset(m, "fish");
	assert(map_get(m, "fish") == NULL);
	assert(!mystrcmp(map_set(m, "", "splunge"),"splunge"));
	assert(map_spareEntries(m) == 167);
	assert(!mystrcmp(map_get(m, "heads"),""));
	assert(!mystrcmp(map_get(m, ""),"splunge"));

	assert(map_set(m, "flib", (void *)4242) == (void *)4242);
	assert(map_get(m, "flib") == (void *)4242);

	return 0;
}
