/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.sort@1: sort twenty numbers and keep the answer.  The first time, there is
 * nothing on the disk, so it saves the sorted line as a file called "sorted".
 * Every time after, it finds that file and checks it against what it just
 * worked out.  Insertion sort, the one in every first course.  The numbers come
 * from the rand() in Kernighan and Ritchie, section 2.7, from its own starting
 * seed of 1: the same twenty every time, on every machine.
 *
 * Asks the door for write, exit, load and save.  Ours.
 */

#include "../PART.loader@5/door.h"

#define N 20

static unsigned long int next = 1;	/* K&R 2.7: rand: return pseudo-random integer on 0..32767 */

static int rand(void)
{
	next = next * 1103515245 + 12345;
	return (unsigned int)(next / 65536) % 32768;
}

static const struct door *d;

static void say(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	d->write(s, n);
}

/* n as decimal digits at p; returns how many */
static unsigned long digits(char *p, unsigned long n)
{
	char t[12];
	unsigned long k = 0, i;

	do
		t[k++] = '0' + n % 10;
	while ((n /= 10) != 0);
	for (i = 0; i < k; i++)
		p[i] = t[k - 1 - i];
	return k;
}

static char line[N * 6 + 2], old[sizeof line + 1], num[12];

void __attribute__((section(".text.start"))) _start(const struct door *door)
{
	int a[N], i, j, v;
	unsigned long len = 0, k;
	long got;

	d = door;
	for (i = 0; i < N; i++)
		a[i] = rand() % 1000;

	/* insertion sort: each number slides left past everything bigger */
	for (i = 1; i < N; i++) {
		v = a[i];
		for (j = i - 1; j >= 0 && a[j] > v; j--)
			a[j + 1] = a[j];
		a[j + 1] = v;
	}

	for (i = 0; i < N; i++) {
		len += digits(line + len, (unsigned long)a[i]);
		line[len++] = i < N - 1 ? ' ' : '\n';
	}
	say("sorted 20 numbers: ");
	d->write(line, len);

	got = d->load("sorted", old, sizeof old);
	if (got < 0) {
		if (d->save("sorted", line, len) == 0) {
			say("nothing saved yet; saved \"sorted\", ");
			d->write(num, digits(num, len));
			say(" bytes\n");
		} else {
			say("could not save \"sorted\"\n");
		}
	} else {
		say("found \"sorted\" from last time, ");
		d->write(num, digits(num, (unsigned long)got));
		say(" bytes: ");
		for (k = 0; k < len && (unsigned long)got == len && old[k] == line[k]; k++)
			;
		say(k == len && (unsigned long)got == len ? "the same\n" : "different\n");
	}
	d->exit(0);
}
