/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.linux-only@1, the old programs' side: a program written for
 * the old system call table asks the kernel, with int 0x80 alone, for four Linux
 * numbers, each with the arguments the old table's alloc, ms, load and save would
 * hand over.  SPEC.gate.10: every call is at Linux's number for it, so a number
 * means Linux's call whoever asks.  An old call hidden behind another call's
 * number (try 6, findings 0024 item 19) answers here as the old call:
 *   85  readlink("one", buf, 64): Linux answers -22 for a file that is not a link,
 *       -2 if there is none; never the file, and buf stays as it was
 *   232 listxattr("x", buf, 64): -2, -61 or -95 from Linux; never a save
 *   265 clock_gettime(0, 0): -14, the answer has nowhere to go; never a count
 *   45  brk(16), twice: below the program's break, so Linux leaves the break
 *       where it is and answers the same both times; never 16 bytes handed out
 * Any of them may instead be -38, a call the gate does not do.  One line each.
 * Boots on: cat.
 */
#include "door.h"

static long sys(long n, long a, long b, long c)
{
	long r;

	__asm__ volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a), "c"(b), "d"(c) : "memory");
	return r;
}

static void line(const char *name, long v, const char *tail)
{
	char b[64], d[12];
	int n = 0, k = 0;
	unsigned long u = v < 0 ? (unsigned long)-v : (unsigned long)v;

	while (*name)
		b[n++] = *name++;
	b[n++] = ' ';
	if (v < 0)
		b[n++] = '-';
	do { d[k++] = (char)('0' + u % 10); u /= 10; } while (u);
	while (k)
		b[n++] = d[--k];
	if (*tail)
		b[n++] = ' ';
	while (*tail)
		b[n++] = *tail++;
	b[n++] = '\n';
	sys(4, 1, (long)b, n);
}

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	static char buf[64];
	long r, r2;
	int i, same = 1;

	(void)d;
	for (i = 0; i < 64; i++)
		buf[i] = 'z';
	r = sys(85, (long)"one", (long)buf, 64);
	for (i = 0; i < 64; i++)
		if (buf[i] != 'z')
			same = 0;
	line("nr-85", r, same ? "untouched" : "written");
	line("nr-232", sys(232, (long)"x", (long)buf, 64), "");
	line("nr-265", sys(265, 0, 0, 0), "");
	r = sys(45, 16, 0, 0);
	r2 = sys(45, 16, 0, 0);
	line("nr-45", 0, r == r2 ? "same" : "moved");
	sys(1, 0, 0, 0);
	for (;;)
		;
}
