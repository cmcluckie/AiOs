/* Probe for TST.share.separate@1: a program that goes looking for someone
 * else's memory.  It reads its own first byte (fine), says where it is about
 * to write, then writes one byte just past the end of its own 8 MiB.  Under
 * loader@6 that address belongs to nobody it can reach: the processor refuses,
 * and the kernel should stop this program alone. */
#include "door.h"

void __attribute__((section(".text.start"))) _start(const struct door *door)
{
	static const char msg[] = "vandal: writing to 0x00c00000\n";
	volatile unsigned char *own = (volatile unsigned char *)0x400000, *past = (volatile unsigned char *)0xc00000;

	(void)*own;
	door->write(msg, sizeof msg - 1);
	*past = 0x42;
	door->write("vandal: got away with it\n", 25);
	door->exit(0);
}
