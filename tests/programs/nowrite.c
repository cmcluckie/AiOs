/* Probe for TST.fs.no-write@1: on the cat flavor's kernel, which was asked to
 * read files and not to write them, tries to save one.  Says "save refused"
 * when save answers -1, "save worked" when it does not.
 * Boots on: cat.
 * Expects refusal: CAP.fs.write.
 * Not a component; never admitted. */
#include "door.h"
void _start(const struct door *d) __attribute__((section(".text.start")));
void _start(const struct door *d)
{
	static const char x[] = "written anyway\n";

	if (d->save("x", x, sizeof x - 1))
		d->write("save refused\n", 13);
	else
		d->write("save worked\n", 12);
	d->exit(0);
}
