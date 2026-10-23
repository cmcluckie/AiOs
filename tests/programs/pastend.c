/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.window-end@1: a plain program on a machine with 62 MiB, so
 * its window, from 4 MiB to the end of RAM, does not end on a 4 MiB line.  It
 * reads the last byte of its window, says so, then reads the first byte past it,
 * which it was never given (TECH-DEBT 3).  A kernel that maps only the window
 * stops it there with a page fault.
 */
#include "door.h"

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	volatile unsigned char *last = (volatile unsigned char *)0x03dfffff;
	volatile unsigned char *past = (volatile unsigned char *)0x03e00000;
	unsigned char b;

	b = *last;
	(void)b;
	d->write("inside\n", 7);
	b = *past;
	d->write("got away\n", 9);
	d->exit(0);
}
