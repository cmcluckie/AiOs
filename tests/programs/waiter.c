/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.share.waits@1: a program that asks for one byte from the console
 * and waits for it.  Nothing is ever typed.  It runs beside the stopwatch, which
 * should keep its time while this one waits (TECH-DEBT 2).
 * Boots on: share, gateshare.
 */
#include "door.h"

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	char c;

	d->write("waiting\n", 8);
	d->read(&c, 1);
	d->write("got a byte\n", 11);
	d->exit(0);
}
