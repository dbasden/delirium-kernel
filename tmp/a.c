#include <stdio.h>
#include <math.h>

long int d2b (int i) {
	long int r;

	r=0;
	while (i != 0) { 
		r *= 10; r += (i % 2); i /= 2; 
	}
	return r;
}

int main() {
	union { float f; int d; } x;

	printf("%ld\n", d2b(255));
	x.f = 0.0001;
	printf("%f %d\n",x.f, x.d);
	x.f = 1000.0001;
	printf("%f %d\n",x.f, x.d);
	x.f = 1000.0;
	printf("%f %d\n",x.f, x.d);

	return 0;
}
