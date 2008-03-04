#ifndef __MAP_H
#define __MAP_H

/*
 * Simple map implementation based on bsearchtree
*
 * Not dependant on any libc, or malloc.
 *
 * Limitations:
 *    - Currently doesn't free any memory, or space in the tree when
 *      a key is removed from the domain.
 *    - Not light on memory (28 bytes per map entry, not including
 *      the actual key and values pointed to)
 *
 * Copyright (c)2005 David Basden
 */

/* Currently ignored, but can be changed if need to change alignment */
#define MAP_ALIGN_TO	4

#ifndef size_t
#define size_t	unsigned int
#endif

#include "bsearchtree.h"

struct map {
	struct bsearchtree  bst;
	compare_fn_t compare_key;
	void *mem;
	void *next_free;
	size_t mem_size;
};
typedef struct map 	map_t;

struct map_item {
	map_t *parent; /* Ugly */
	void *key;
	void *value;
};
typedef struct map_item map_item_t;

#define MAP_KEY(_a) (((map_item_t *)(_a))->key)
#define MAP_VAL(_a) (((map_item_t *)(_a))->value)
#define MAP_PARENT(_a) (((map_item_t *)(_a))->parent)
#define MAP_MEM_FREE(_map_p) ( (_map_p)->mem_size - ((_map_p)->next_free - (_map_p)->mem))
#define MAP_ALIGN_SIZE(_t) sizeof(_t)

int map_compare(void *a, void *b);

/* Initialise a map with a block of memory and a cmp function */
map_t map_new(void *mem, size_t mem_size, compare_fn_t compare_key);

/* Return the amount of entries that can be put in the map */
size_t map_spareEntries(map_t *self);

/* Returns value in co-domain for key, or null iff key is not in
 * map domain */
void *map_get(map_t *self, void *key);

/* Set key to value. If key was not in the map, it is added 
 * returns value, or NULL iff the key could not be set */
void *map_set(map_t *self, void *key, void *value);

void map_unset(map_t *self, void *key);

#endif
