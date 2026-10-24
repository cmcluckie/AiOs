/* Probe for TST.loader.alloc-in-ram@1 and TST.loader.alloc-refuses@1.  Asks for
 * 128 MiB (more than the machine has), then 1 MiB at a time until refused, then
 * 16 bytes.  Prints every answer, one per line, in hex:
 *   big ADDR          the 128 MiB request
 *   block ADDR SIZE   each 1 MiB block granted
 *   refused SIZE      the first 1 MiB request refused
 *   small ADDR        the 16-byte request after the refusal
 *   end               the program got here, so nothing halted it
 * Not a component; never admitted.
 * Boots on: life.
 */
#include "door.h"
static const struct door *d;
static void hex(unsigned long v)
{
	char s[11];
	int i;

	s[0] = '0'; s[1] = 'x';
	for (i = 0; i < 8; i++)
		s[2 + i] = "0123456789abcdef"[(v >> (28 - 4 * i)) & 15];
	s[10] = ' ';
	d->write(s, 11);
}
static void put(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	d->write(s, n);
}
void __attribute__((section(".text.start"), noreturn)) _start(const struct door *door)
{
	void *p;

	d = door;
	put("big "); hex((unsigned long)d->alloc(128UL << 20)); put("\n");
	while ((p = d->alloc(1UL << 20)) != 0) {
		put("block "); hex((unsigned long)p); hex(1UL << 20); put("\n");
	}
	put("refused "); hex(1UL << 20); put("\n");
	put("small "); hex((unsigned long)d->alloc(16)); put("\n");
	put("end\n");
	d->exit(0);
}
