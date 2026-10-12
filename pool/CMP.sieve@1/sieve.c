/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.sieve@1: the sieve of Eratosthenes as BYTE magazine ran it in 1981 to
 * compare the home computers of the day: 8191 flags, odd numbers only, 1899
 * primes.  This one runs it RUNS times flat out and never once gives the
 * processor back while it works: no door call between the first run and the
 * last.  On a machine that only shares when a program offers, nothing else
 * would run until it finished.  Then it says how long that took, by the door.
 *
 * Asks the door for write, ms and exit.  Ours.
 */
#include "../CMP.loader@6/door.h"

#define SIZE 8190
#define RUNS 800

static char flags[SIZE + 1];

static unsigned long digits(char *p, unsigned long n)
{
	char t[12];
	unsigned long k = 0, i;

	do
		t[k++] = (char)('0' + n % 10);
	while ((n /= 10) != 0);
	for (i = 0; i < k; i++)
		p[i] = t[k - 1 - i];
	return k;
}

void __attribute__((section(".text.start"))) _start(const struct door *door)
{
	static char line[80];
	unsigned long start, ms, len = 0;
	int i, k, prime, count = 0, run;
	const char *s;

	start = door->ms();
	for (run = 0; run < RUNS; run++) {
		count = 0;
		for (i = 0; i <= SIZE; i++)
			flags[i] = 1;
		for (i = 0; i <= SIZE; i++) {
			if (flags[i]) {
				prime = i + i + 3;
				for (k = i + prime; k <= SIZE; k += prime)
					flags[k] = 0;
				count++;
			}
		}
	}
	ms = door->ms() - start;

	for (s = "\nBYTE sieve: "; *s; s++)
		line[len++] = *s;
	len += digits(line + len, (unsigned long)count);
	for (s = " primes, "; *s; s++)
		line[len++] = *s;
	len += digits(line + len, RUNS);
	for (s = " times over, in "; *s; s++)
		line[len++] = *s;
	len += digits(line + len, ms);
	for (s = " ms\n"; *s; s++)
		line[len++] = *s;
	door->write(line, len);
	door->exit(0);
}
