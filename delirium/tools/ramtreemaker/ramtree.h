#ifndef __RAMTREE_H
#define __RAMTREE_H

/*
 * Delirium ramtree map
 *
 * At this point, delirium doesn't have filesystems. It does
 * have an internal tree for messaging (soapboxes), and this
 * will later develop into more of a database than a filesystem
 * if needed.
 *
 * The ramtree map is a way to get chunks of data into the
 * operating system without having to pass them individually 
 * into the bootloader as "modules". This is especially helpful
 * as we can map them straight into our soapbox namespace, for
 * easy access.
 *
 * Although the format can be used like a simple filesystem,
 * it lacks most of the normal filesystem ideas, which is good
 * for us because we don't want them, or the overhead. It's write-
 * once, and that's the way it stays.
 */

#define RAMTREE_MUNDANE		0xfeedbabe

#ifndef size_t
typedef	unsigned int	size_t;
#endif

struct ramtree_entry {
	size_t	start; 	// Offset into ramtree of data
	size_t	length; // Size of data
	size_t	name;	// Offset into ramtree of null-terminated string
};

struct ramtree {
	size_t			mundane;	// Not magic.
	size_t			total_size;
	size_t			total_entries;
	struct ramtree_entry	entries[];
};


#endif
