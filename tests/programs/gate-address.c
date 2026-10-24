/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.refuses-address@1: a program that hands the kernel an
 * address that is the kernel's own.  write(1, kernel, 16) would put the
 * kernel's bytes on the console and read(0, kernel, 16) would let input
 * overwrite the kernel; both must come back -14 (EFAULT).  The probe says what
 * it was answered with, using only addresses of its own.
 * Boots on: gate.
 */
#include "door.h"

static long sys3(long n, long a, long b, long c)
{
	long r;

	__asm__ volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a), "c"(b), "d"(c) : "memory");
	return r;
}

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	static const char yes[] = "address refused\n";
	static const char no[] = "address accepted\n";
	long w = sys3(4, 1, 0x100000, 16);
	long r = sys3(3, 0, 0x100000, 16);
	const char *m = (w == -14 && r == -14) ? yes : no;
	unsigned long n = (w == -14 && r == -14) ? sizeof yes - 1 : sizeof no - 1;

	(void)d;
	sys3(4, 1, (long)m, (long)n);
	sys3(1, 0, 0, 0);
	for (;;)
		;
}
