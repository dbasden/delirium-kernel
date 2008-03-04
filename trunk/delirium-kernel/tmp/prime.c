/*
 * define a prime number as a number that doesn't have any lower primes as a factor
 */

#include <stdio.h>
#include <stdlib.h>

#include "prime.h"



/*************************[ Storage and memory handling ] ***********************/

/*
 * add a prime to the list
 */
void prime_add(prime_t * primes, prime_t newP) {

	if (primes[PRIMES_FOUND] >= MAX_FOUND) {
		fprintf(stderr, "Found a prime, but couldn't store it. Man, that sucks\n");
		return;
	}

	primes[PRIMES_FOUND]++;
	primes[primes[PRIMES_FOUND]] = newP;
}

/*
 * dump out the listed primes
 */
void prime_dump(prime_t * primes) {
	prime_t i;

	for (i=FIRST_PRIME; i<=primes[PRIMES_FOUND]; i++)
		printf("%u\n", primes[i]);

#if 0
	printf("\n(%llu found)\n\n",primes[PRIMES_FOUND]);
#endif
}

/*
 * Allocate the data structure for the primes to be stored in
 */
prime_t * prime_init() {
	prime_t *primes;

	primes = malloc((MAX_FOUND+1) * sizeof(prime_t));
	if (primes == NULL) {
		perror("allocating memory for prime storage");
		exit(-1);
	}

	primes[PRIMES_FOUND] = 0;

	return primes;
}

/*
 * Clean up datastructures
 */
void prime_cleanup (prime_t *primes) {
	free(primes);
}
