/* SPDX-License-Identifier: MIT
 *
 * CMP.hello@1.  PUR.0000: write one line to the serial console and halt.
 * Requires CAP.tty.write and CAP.proc.exit, both reached through the loader's
 * door and nothing else.  No libc, no hardware, no return.
 */

#include "../CMP.loader@1/door.h"

static const char message[] = "Hello, world.\n";

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *door)
{
	door->write(message, sizeof(message) - 1);	/* 14 bytes, SPEC.hello.1 */
	door->exit(0);
}
