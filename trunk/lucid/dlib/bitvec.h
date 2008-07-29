#ifndef __BITVEC_H
#define __BITVEC_H

/*
 * Simple tools for manipulating bit vectors.
 *
 * The vectors themselves are made of 32 bit words
 *
 * David Basden <davidb-delirium@rcpt.to>
 */

#ifndef __INT_TYPES_DEFINED

#ifndef ARCH_lucid
#ifndef u_int32_t
typedef unsigned int u_int32_t;
#endif
#endif

#endif
typedef 	u_int32_t *	bitvec_t;

/* Size of bitvec in bytes from bits. Always pad out to nearest 32 bits */
#define BITVEC_SIZE(_bitlen)	( ( ((_bitlen) >> 5) << 2 ) + ( ((0x1f & (_bitlen)) == 0) ? 0 : 4 ) )


/* Which u_int32_t is a specific bit stored in? */
#define BITVEC_OFFS(_vec, _bit)	( *( (_vec) + ((_bit) / 32) ) )

/* Which bit within that offset is the bit we are looking for? */
#define BITVEC_MASK(_bit)	(1 << ((_bit) % 32))


/* Is a bit on? */
#define BITVEC_GET(_vec, _bit)		(BITVEC_MASK(_bit) & BITVEC_OFFS(_vec, _bit))

/* Set bit on */
#define BITVEC_SET(_vec, _bit)		(BITVEC_OFFS(_vec, _bit) = BITVEC_OFFS(_vec, _bit) | BITVEC_MASK(_bit))

/* Set bit off */
#define BITVEC_CLEAR(_vec, _bit)	(BITVEC_OFFS(_vec, _bit) = BITVEC_OFFS(_vec, _bit) & (~BITVEC_MASK(_bit)))

/* Invert bit */
#define BITVEC_FLIP(_vec, _bit)		(BITVEC_OFFS(_vec, _bit) = BITVEC_OFFS(_vec, _bit) ^ BITVEC_MASK(_bit))

#endif /* __BITVEC_H */
