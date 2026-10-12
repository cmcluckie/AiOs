/* Probe for TST.loader.ms@1 and TST.timer.rate@1: print what the ms door says,
 * one line every 500 ms by the door, forever.  jig/test stamps each line with
 * the host's clock and compares the two.  Not a component; never admitted. */
#include "door.h"
static const struct door *d;
static void num(unsigned long v)
{
	char b[12];
	int i = 11;

	b[i] = '\n';
	do { b[--i] = (char)('0' + v % 10); v /= 10; } while (v);
	d->write(b + i, (unsigned long)(12 - i));
}
void __attribute__((section(".text.start"), noreturn)) _start(const struct door *door)
{
	unsigned long next = 0, now;

	d = door;
	for (;;) {
		now = d->ms();
		if (now >= next) {
			num(now);
			next += 500;
		}
	}
}
