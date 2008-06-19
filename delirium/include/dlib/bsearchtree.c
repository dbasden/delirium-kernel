#include "btree.h"
#include "bsearchtree.h"

/*
 * simple BSearchTree implementation
 */

bsearchtree_t bsearchtree_new(compare_fn_t compare) {
	bsearchtree_t bst;

	bst.compare = compare;
	bst.tree = NULL;

	return bst;
}

int bsearchtree_isEmpty(bsearchtree_t *bst) {
	return bst->tree == NULL;
}

/*
 * return the subtree of subtree *tree  where an item would be located
 * or inserted
 *
 * pre: tree is not NULL
 */
btree_t *bsearchtree_seek(bsearchtree_t *self, btree_t *tree, void *item) {
	int compareval;

	for (;;) {
		compareval = (self->compare)(item, tree->item);
		if (compareval < 0 && HAS_LEFT(tree)) {
			tree = tree->left;
		} else if (compareval > 0 && HAS_RIGHT(tree)) {
			tree = tree->right;
		} else {
			break;
		}
	}
	return tree;
}

/* 
 * Search for an item in the tree. If it is found, return the
 * item in the tree. If not, return NULL
 */
void * bsearchtree_find(bsearchtree_t *self, void *item) {
	btree_t * tree;

	if (bsearchtree_isEmpty(self)) return NULL;
	tree = bsearchtree_seek(self, self->tree, item);
	if (self->compare(item, tree->item) == 0) return tree->item;
	return NULL;
}

/*
 * add a tree item in the correct place in the tree
 * pre: tree doesn't already exist in the bsearchtree
 *
 * returns entry iff inserted
 * returns NULL iff the entry already exists in the bsearchtree
 */
void *bsearchtree_add(bsearchtree_t *self, btree_t *entry) {
	int compareResult;
	btree_t *t;
	
	t = self->tree;


	if (bsearchtree_isEmpty(self)) {
		self->tree = entry;
		entry->left = NULL;
		entry->right = NULL;
		return entry;
	}

	t = bsearchtree_seek(self, t, entry->item);
	compareResult = self->compare(entry->item, t->item);

	if (compareResult == 0) return NULL;
	if (compareResult < 0) { t->left = entry; } 
	else { t->right = entry; }

	entry->left = NULL;
	entry->right = NULL;
	return entry;
}
