#include <stdio.h>
#include <stdint.h>

#include "timing.h"
#include "prime.h"


void stupidFindPrimes() {
	prime_t *primes;
	prime_t candidate;
	prime_t f;

	primes = prime_init();

	for (candidate=MIN_CHECKED; candidate<MAX_CHECKED; candidate++) {

		/* 
		 * Break out if we find a factor f
		 * where 1 < f < candidate
		 */
		for (f=2; f<candidate; f++)  {
			if (IS_FACTOR(candidate, f)) 
				break;
		}

		if (f == candidate)  {
			prime_add(primes, candidate);
		}
	}

	prime_dump(primes);
	prime_cleanup(primes);
}

inline void checkPrime(prime_t *primes, prime_t candidate) {
		prime_t factor;

		for (factor=3u; factor<candidate; factor = factor + 2u)
			if (IS_FACTOR(candidate, factor) == 0)
				break;

		if (factor >= candidate) 
			prime_add(primes, candidate);
}


inline void goodCheckPrime(prime_t *primes, prime_t candidate) {
	prime_t i;
	prime_t max_to_check = candidate/2;
	// Only check primes < candidate/2
	for (i=FIRST_PRIME; i<=primes[PRIMES_FOUND] && primes[i] <= max_to_check ; i++)
		if (DBL_IS_FACTOR(candidate, primes[i]) == 0) {
			i=candidate;
			break;
		}

	if (i != candidate)
		prime_add(primes, candidate);
}


void lessStupidFindPrimes() {
	prime_t *primes;
	prime_t candidate;

	primes = prime_init();

	// Can never be an even number, apart from 2
	prime_add(primes, 2u);
	prime_add(primes, 3u);
	prime_add(primes, 5u);

	for (candidate=7u; candidate <=MAX_CHECKED; candidate += 2u) {
		/*
		 * Throw out anything that isn't a factor of 6, +/- 1
		 */
		if (candidate % 6u == 5u || candidate % 6u == 1u) 
			checkPrime(primes, candidate);
	}

	prime_dump(primes);
	prime_cleanup(primes);
}


void okayFindPrimes() {
	prime_t *primes;
	prime_t candidate;

	primes = prime_init();

	// Seed
	prime_add(primes, 2u); prime_add(primes, 3u);

	// Iterate through every multiple of 6, and check +/- 1
	for (candidate=6u; candidate < MAX_CHECKED; candidate += 6u) {
		goodCheckPrime(primes, candidate-1);
		goodCheckPrime(primes, candidate+1);
	}

	prime_dump(primes);
	prime_cleanup(primes);
}


int main(int argc, char **argv) {

	printCurrentCPU();
	//stupidFindPrimes();
	//lessStupidFindPrimes();
	okayFindPrimes();
	printCurrentCPU();


	return 0;	
}
