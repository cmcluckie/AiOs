/* Probe for TST.serial.in-order@1: hand the write door all 256 byte values in
 * order, in one call, then one byte at a time in reverse.  Not a component.
 * Boots on: hello.
 */
#include "door.h"
void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	static unsigned char b[256];
	int i;

	for (i = 0; i < 256; i++)
		b[i] = (unsigned char)i;
	d->write(b, 256);
	for (i = 255; i >= 0; i--)
		d->write(&b[i], 1);
	d->exit(0);
}
