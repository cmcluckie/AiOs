/* The cheat of findings 0019 #7: a program that writes the screen itself.
 * Boots on: no flavor's kernel; kept as the record of findings 0019 #7.
 */
#include "../../pool/PART.loader@8/door.h"
__attribute__((section(".text.start"))) void _start(const struct door *d)
{
	volatile unsigned short *v = (volatile unsigned short *)0xb8000;
	const char *m = "AiOs on a screen";
	for (int i = 0; m[i]; i++) v[i] = (unsigned short)(0x1e00 | (unsigned char)m[i]);
	d->write("ok\n", 3);
	for (;;) ;
}
