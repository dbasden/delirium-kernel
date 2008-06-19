#ifndef _PRIME_CALC_H
#define _PRIME_CALC_H
#include <math.h>

/*
 * IS_FACTOR(a,b) returns true iff b is a multiple of a
 * e.g. IS_FACTOR(10,2) == 0
 *      IS_FACTOR(10,3) != 0
 */
#define IS_FACTOR(a,b) ((a) % (b))
#define DBL_IS_FACTOR(a,b) ((int)fmod((double)(a),(double)(b)))

#endif
