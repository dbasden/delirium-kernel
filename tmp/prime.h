#ifndef __PRIME_H
#define __PRIME_H

#include <limits.h>

#include <stdint.h>

#include "prime_calc.h"

/* 
 * How many numbers we search through for primes
 * 	2^8 = 18446744073709551616
 *	(assuming we are using a uint64_t)
 */
#define MIN_CHECKED (prime_t)(2u)
#if 0
#define MAX_CHECKED (prime_t)(18446744073709551615ULL)
#endif
#define MAX_CHECKED (prime_t)(200000ULL)


/*
 * Type we use to hold numbers. 64bit unsigned int
 */
//typedef uint64_t prime_t __attribute__ ((mode(SI)));
typedef uint32_t prime_t;

/*
 * How many primes we allocate memory to store.
 * Later, we'll just malloc and grow as we need,
 * and let the MMU deal with the hard bit
 */
#define MAX_FOUND 655360

/*
 * The index of the primes found to date in the primes array,
 * and the index of the first prime in the array
 */
#define PRIMES_FOUND 0
#define FIRST_PRIME 1

/*
 * Headers
 */
void prime_add(prime_t * primes, prime_t newP);
prime_t * prime_init();
void prime_cleanup (prime_t *primes);
void prime_dump(prime_t * primes);

#endif
