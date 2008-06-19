/*
 * Simple map implementation based on bsearchtree
*
 * Not dependant on any libc, or malloc.
 *
 * Limitations:
 *    - Currently doesn't free any memory, or space in the tree when
 *      a key is removed from the domain.
 *    - Not light on memory
 *
 * Copyright (c)2005 David Basden
 */

#include "map.h"
#include "bsearchtree.h"

int map_compare(void *a, void *b) {
	return (((map_item_t *)b)->parent->compare_key(MAP_KEY(a), MAP_KEY(b)));
}

/* Initialise a map with a block of memory and a cmp function */
map_t map_new(void *mem, size_t mem_size, compare_fn_t compare_key) {
	map_t self;

	self.compare_key = compare_key;
	self.bst = bsearchtree_new(map_compare);
	self.mem = mem;
	self.next_free = mem;
	self.mem_size = mem_size;

	return self;
}


/* Return the amount of entries that can be put in the map */
size_t map_spareEntries(map_t *self) {
	return (MAP_MEM_FREE(self) / (MAP_ALIGN_SIZE(btree_t) + MAP_ALIGN_SIZE(map_item_t)));
}

/* Get a new btree link with a map entry already allocated */
static btree_t *new_map_entry(map_t *self) {
	btree_t *bt;

	if (!map_spareEntries(self)) return NULL;
	bt = self->next_free;
	self->next_free = (char *)(self->next_free) + MAP_ALIGN_SIZE(btree_t);
	bt->item = self->next_free;
	self->next_free = (char *)(self->next_free) + MAP_ALIGN_SIZE(map_item_t);

	return bt;
}

/* Returns value in co-domain for key, or null iff key is not in
 * map domain */
void *map_get(map_t *self, void *key) {
	map_item_t searchfor;
	map_item_t *found;

	searchfor.key = key;
	found = bsearchtree_find(&(self->bst), &searchfor);
	return (found == NULL) ? NULL : (found->value);
}

/* Set key to value. If key was not in the map, it is added 
 * returns value, or NULL iff the key could not be set */
void *map_set(map_t *self, void *key, void *value) {
	map_item_t searchfor;
	map_item_t *found;

	searchfor.key = key;
	found = bsearchtree_find(&(self->bst), &searchfor);

	if (found == NULL) {
		btree_t *newentry;

		newentry = new_map_entry(self);
		if (newentry == NULL) return NULL;
		MAP_KEY(newentry->item) = key;
		MAP_VAL(newentry->item) = value;
		MAP_PARENT(newentry->item) = self;
		newentry = bsearchtree_add(&(self->bst), newentry);
		if (newentry == NULL) return NULL;
		return MAP_VAL(newentry->item);
	}
	found->value = value;
	return found->value;
}

void map_unset(map_t *self, void *key) {
	map_set(self, key, NULL);
}
