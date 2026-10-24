/* Probe for TST.serial.receive-order@1: read 4096 bytes through the read door and
 * write every one straight back.  Not a component; never admitted; built only by
 * jig/test against the loader under test.
 * Boots on: calc.
 */
#include "door.h"
#define N 4096
void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	static unsigned char buf[256];
	unsigned long got = 0, n;

	while (got < N) {
		n = d->read(buf, N - got < sizeof buf ? N - got : sizeof buf);
		d->write(buf, n);
		got += n;
	}
	d->exit(0);
}
