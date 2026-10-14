/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.fs.overwrite-clean@1: one file saved five times over, at sizes
 * that make PART.fs.minix@2 take zones, give them back, fill all seven, empty
 * the file, and grow it again.  Says how each save went, then loads the file
 * and says whether it is byte for byte the last thing saved.
 */
#include "door.h"

static const struct door *d;
static char buf[7168], back[7168];

static void say(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	d->write(s, n);
}

static void num(unsigned long n)
{
	char t[12];
	int k = 0;

	do
		t[k++] = '0' + n % 10;
	while ((n /= 10) != 0);
	while (k)
		d->write(&t[--k], 1);
}

static void fill(unsigned long n, unsigned long seed)
{
	unsigned long i;

	for (i = 0; i < n; i++)
		buf[i] = (char)('a' + (i * 7 + seed) % 26);
}

void __attribute__((section(".text.start"))) _start(const struct door *door)
{
	static const unsigned long size[] = { 2500, 10, 7168, 0, 1500 };
	unsigned long i, n = 0;
	long got;

	d = door;
	for (i = 0; i < 5; i++) {
		n = size[i];
		fill(n, i);
		say("save ");
		num(n);
		say(d->save("notes.txt", buf, n) == 0 ? ": ok\n" : ": failed\n");
	}
	got = d->load("notes.txt", back, sizeof back);
	say("load: ");
	if (got < 0) {
		say("failed\n");
	} else {
		num((unsigned long)got);
		for (i = 0; i < n && (long)i < got && back[i] == buf[i]; i++)
			;
		say((unsigned long)got == n && i == n ? " bytes, the same\n" : " bytes, different\n");
	}
	d->exit(0);
}
