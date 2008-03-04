#ifndef __BSEARCHTREE_H
#define __BSEARCHTREE_H

#include "btree.h"

#ifndef NULL
#define NULL	((void *)0)
#endif

/*
 * simple BSearchTree implementation
 *
 * parent always exists, but has item set to NULL if tree is totally empty
 */

typedef int(*compare_fn_t)(void *a, void *b);

struct bsearchtree {
	btree_t *tree;
	compare_fn_t compare;
};
typedef struct bsearchtree bsearchtree_t;


bsearchtree_t bsearchtree_new(compare_fn_t);

int bsearchtree_isEmpty(bsearchtree_t *bst);

/*
 * return the subtree of subtree *tree  where an item would be located
 * or inserted
 *
 * pre: tree is not NULL
 */
btree_t *bsearchtree_seek(bsearchtree_t *self, btree_t *tree, void *item);

/* 
 * Search for an item in the tree. If it is found, return the
 * item in the tree. If not, return NULL
 */
void * bsearchtree_find(bsearchtree_t *self, void *item);

/*
 * add an item into the bsearch tree.
 * Passed in is a btree_t item to place in the tree. The only
 * thing left unchanged will be tree_entry->item
 */
void * bsearchtree_add(bsearchtree_t *self, btree_t *tree_entry);

#endif
