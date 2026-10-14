/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.stopwatch@1.  PUR.0004: count up, once a second, from 00:00, on one line
 * of the serial console, until the machine is switched off.
 *
 * The clock chapter.  Requires CAP.tty.write and CAP.time.read, nothing else.
 * Two rules from the turbo-button years:
 *   - never time anything by counting your own work: the time comes from the
 *     door, in milliseconds, never from how many times a loop ran;
 *   - aim every tick at start + n seconds, never at "a second after the last
 *     one", so a late redraw is caught up by the next and nothing drifts.
 * Waiting is asking the door "is it time yet?" in a loop.  No interrupts
 * exist yet, and with one program nothing else wants the processor.
 */
#include "../PART.loader@4/door.h"

static const struct door *door;

static void show(unsigned long seconds)
{
	char t[6];
	unsigned long m = seconds / 60 % 100, s = seconds % 60;

	t[0] = '\r';
	t[1] = (char)('0' + m / 10); t[2] = (char)('0' + m % 10);
	t[3] = ':';
	t[4] = (char)('0' + s / 10); t[5] = (char)('0' + s % 10);
	door->write(t, sizeof t);
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	unsigned long start, n;

	door = d;
	start = door->ms();
	for (n = 0; ; n++) {
		while (door->ms() - start < n * 1000)
			;			/* not yet */
		show(n);
	}
}
