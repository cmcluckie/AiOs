/* Probe for TST.cat.*@1: puts the files cat is asked to print on the disk.  It
 * runs on the editor's kernel, whose petition (0007) asks to write files; cat's
 * asks only to read them, so cat's kernel has no save (decision 0065):
 *   one   two lines
 *   two   one line
 *   big   5,000 bytes: "line 0001\n" to "line 0500\n", more than cat's buffer
 * Says "files written" when all three were saved, "save failed" otherwise.
 * Not a component; never admitted.
 * Boots on: editor.
 */
#include "door.h"
static char big[5000];
void _start(const struct door *d) __attribute__((section(".text.start")));
void _start(const struct door *d)
{
	static const char one[] = "the first file\nsecond line of it\n";
	static const char two[] = "the second file\n";
	unsigned int i, n;

	for (i = 0; i < 500; i++) {
		char *p = big + 10 * i;

		n = i + 1;
		p[0] = 'l'; p[1] = 'i'; p[2] = 'n'; p[3] = 'e'; p[4] = ' ';
		p[5] = (char)('0' + n / 1000 % 10); p[6] = (char)('0' + n / 100 % 10);
		p[7] = (char)('0' + n / 10 % 10); p[8] = (char)('0' + n % 10); p[9] = '\n';
	}
	if (d->save("one", one, sizeof one - 1) || d->save("two", two, sizeof two - 1) || d->save("big", big, sizeof big))
		d->write("save failed\n", 12);
	else
		d->write("files written\n", 14);
	d->exit(0);
}
