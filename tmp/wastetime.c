#include <stdio.h>

int main() {
	double a = 1;
	double fishy1 = 0.1;
	double fishy2 = 0.2;
	int i,j;

	for (j=0; j<1000; j++)
		for (i=0; i<10000000; i++) { 
			a = fishy1 + fishy2;
			a *= fishy1 * fishy2;
			a = fishy1 + fishy2;
			a *= fishy1 * fishy2;
		}

	return 0;
}
