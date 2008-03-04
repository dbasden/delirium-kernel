#ifndef __BTREE_H
#define __BTREE_H

/*
 * simple btree structure
 */

struct btree {
	struct btree	* left;
	struct btree 	* right;
	void *		item;
};
typedef struct btree btree_t;

#define HAS_LEFT(_node)		((_node->left) != NULL)
#define HAS_RIGHT(_node)	((_node->right) != NULL)

#endif
