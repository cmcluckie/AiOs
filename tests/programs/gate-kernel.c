/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.kernel-refused@1: a program that reaches into the kernel's
 * own memory, at 1 MiB where the kernel is loaded.  It reads the byte and
 * writes the same value back, so a kernel that lets it through is not damaged.
 */
#include "door.h"

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	volatile unsigned char *k = (volatile unsigned char *)0x100000;
	unsigned char b;

	d->write("before\n", 7);
	b = *k;
	*k = b;
	d->write("got away\n", 9);
	d->exit(0);
}
