/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Probe for TST.gate.int80@1: a program that never touches the system call
 * table it is handed.  It asks for write (4) and exit (1) with int 0x80 alone,
 * the way a program built for Linux does: number in eax, arguments in ebx, ecx
 * and edx, file 1 the console.
 * Boots on: gate.
 */
#include "door.h"

void __attribute__((section(".text.start"))) _start(const struct door *d)
{
	static const char s[] = "int80 ok\n";
	long r;

	(void)d;
	__asm__ volatile("int $0x80" : "=a"(r) : "0"(4), "b"(1), "c"(s), "d"(sizeof s - 1) : "memory");
	__asm__ volatile("int $0x80" : : "a"(1), "b"(0) : "memory");
	for (;;)
		;
}
