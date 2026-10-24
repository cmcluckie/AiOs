/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.port-refused@1: a program that writes bytes to the serial
 * port with outb instead of asking the kernel to.  The bytes spell PORTCHEAT,
 * which nothing else in the capture contains.
 * Boots on: gate, gateshare.
 */
#include "door.h"

static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	static const char s[] = "PORTCHEAT\n";

	d->write("before\n", 7);
	for (int i = 0; s[i]; i++)
		outb((unsigned char)s[i], 0x3f8);
	d->write("got away\n", 9);
	d->exit(0);
}
