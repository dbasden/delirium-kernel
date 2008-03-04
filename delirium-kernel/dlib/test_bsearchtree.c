#include <stdio.h>
#include <assert.h>
#include "bsearchtree.h"

int mycompare(void *a, void *b) {
	return *((int *)a) - *((int *)b);
}

int main() {
	bsearchtree_t bst;
	bsearchtree_t * bstp;
	btree_t bta;
	btree_t btb;
	btree_t btc;
	btree_t btd;
	btree_t bte;
	btree_t btf;
	int a = 0;
	int b = 1;
	int c = 2;
	int d = 3;
	int e = 4;
	int f = 5;
	void *ap = &a;
	void *bp = &b;
	void *cp = &c;
	void *dp = &d;
	void *ep = &e;
	void *fp = &f;

	bst = bsearchtree_new(mycompare);
	bstp = &bst;

	assert(bsearchtree_isEmpty(bstp));
	assert(bsearchtree_find(bstp, dp) == NULL);
	assert(d == 3);

	btd.item = dp;
	assert(bsearchtree_add(bstp, &btd) == &btd);
	assert(!bsearchtree_isEmpty(bstp));
	assert(bsearchtree_find(bstp, dp) == dp);
	assert(bsearchtree_find(bstp, bp) == NULL);

	bta.item = ap;
	assert(bsearchtree_add(bstp, &bta) == &bta);
	assert(bsearchtree_add(bstp, &bta) == NULL);
	assert(bsearchtree_add(bstp, &btd) == NULL);
	assert(!bsearchtree_isEmpty(bstp));
	assert(bsearchtree_find(bstp, dp) == dp);
	assert(bsearchtree_find(bstp, bp) == NULL);

	btb.item = bp;
	assert(bsearchtree_add(bstp, &btb) == &btb);
	btc.item = cp;
	assert(bsearchtree_add(bstp, &btc) == &btc);
	bte.item = ep;
	assert(bsearchtree_add(bstp, &bte) == &bte);
	btf.item = fp;
	assert(bsearchtree_add(bstp, &btf) == &btf);

	assert(!bsearchtree_isEmpty(bstp));
	assert(bsearchtree_add(bstp, &bta) == NULL);
	assert(bsearchtree_add(bstp, &btb) == NULL);
	assert(bsearchtree_add(bstp, &btc) == NULL);
	assert(bsearchtree_add(bstp, &btd) == NULL);
	assert(bsearchtree_add(bstp, &bte) == NULL);
	assert(bsearchtree_add(bstp, &btf) == NULL);
	assert(bsearchtree_find(bstp, ap) == ap);
	assert(bsearchtree_find(bstp, bp) == bp);
	assert(bsearchtree_find(bstp, cp) == cp);
	assert(bsearchtree_find(bstp, dp) == dp);
	assert(bsearchtree_find(bstp, ep) == ep);
	assert(bsearchtree_find(bstp, fp) == fp);
	return 0;
}
