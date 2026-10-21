/* The divide helper against forty divisions worked out beforehand with 64-bit
 * arithmetic: the edges (by one, by itself, the largest number, across the 32-bit
 * line) and random ones.  Prints the first wrong one and exits 1, or exits 0. */
#include <stdio.h>
#include "cases.h"
unsigned long long __udivmoddi4(unsigned long long, unsigned long long, unsigned long long *);
int main(void)
{
	unsigned i, n = sizeof cases / sizeof cases[0];
	for (i = 0; i < n; i++) {
		unsigned long long r = 7, q = __udivmoddi4(cases[i][0], cases[i][1], &r);
		if (q != cases[i][2] || r != cases[i][3]) {
			printf("%llu / %llu gave %llu remainder %llu, not %llu remainder %llu\n", cases[i][0], cases[i][1], q, r, cases[i][2], cases[i][3]);
			return 1;
		}
	}
	printf("%u divisions exact\n", n);
	return 0;
}
