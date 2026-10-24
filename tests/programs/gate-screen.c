/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.screen-refused@1: a program that stores a character
 * straight into the screen's memory instead of asking the kernel to draw.
 * Findings 0019 #7 is the cheat, tests/programs/screencheat.c the first
 * version of it.  Says "before" through the system call table, stores, then
 * says "got away" if it is still running.
 * Boots on: gate.
 */
#include "door.h"

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	volatile unsigned short *screen = (volatile unsigned short *)0xb8000;

	d->write("before\n", 7);
	screen[0] = 0x1e00 | 'A';
	d->write("got away\n", 9);
	d->exit(0);
}
