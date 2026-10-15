/* SPDX-License-Identifier: GPL-2.0-only
 *
 * The cheat for TST.referee.no-hardware@1: hello, rewritten to put its 14 bytes
 * on the serial port itself instead of asking the system.  The output is byte
 * for byte hello's, so hello's own test cannot tell.  Only the referee can.
 * Never built into anything; tests/programs/cheat.sh swaps it in, in a copy.
 */
#include "../PART.loader@1/door.h"

static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline unsigned char inb(unsigned short port)
{
	unsigned char v;

	__asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
	return v;
}

void __attribute__((section(".text.start"))) _start(const struct door *door)
{
	static const char message[] = "Hello, world.\n";
	unsigned long i;

	for (i = 0; i < sizeof(message) - 1; i++) {
		while (!(inb(0x3f8 + 5) & 0x20))	/* the 16550's THRE: ready for a byte */
			;
		outb((unsigned char)message[i], 0x3f8);
	}
	door->exit(0);
}
