/* Probe for TST.vga.plain@1: thirty plain lines, "line 1" to "line 30", no
 * escapes, then wait for a key.  The screen holds 25, so the first five have
 * scrolled off the top by the time the tester looks.
 * Boots on: screen.
 */
#include "door.h"

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	char buf[16] = "line ", key;
	int i, n;

	for (i = 1; i <= 30; i++) {
		n = 5;
		if (i >= 10)
			buf[n++] = (char)('0' + i / 10);
		buf[n++] = (char)('0' + i % 10);
		buf[n++] = '\n';
		d->write(buf, (unsigned long)n);
	}
	d->read(&key, 1);
	d->exit(0);
}
