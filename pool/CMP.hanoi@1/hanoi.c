/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.hanoi@1.  PUR.0001: print the moves that solve the Towers of Hanoi for
 * four disks, then halt.  Recursion and output.  Requires CAP.tty.write and
 * CAP.proc.exit through the loader's door, and nothing the hello world did not.
 *
 * The problem every CS student solved once: move n disks from peg A to peg C
 * using peg B, one disk at a time, never a larger disk on a smaller one.
 * The solution is 2^n - 1 moves.
 */

#include "../CMP.loader@1/door.h"

#define DISKS 4

static const struct door *door;

static void move(int disk, char from, char to)
{
	char line[] = "Move disk ? from ? to ?\n";

	line[10] = (char)('0' + disk);
	line[17] = from;
	line[22] = to;
	door->write(line, sizeof(line) - 1);
}

static void hanoi(int n, char from, char via, char to)
{
	if (n == 0)
		return;
	hanoi(n - 1, from, to, via);
	move(n, from, to);
	hanoi(n - 1, via, from, to);
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	door = d;
	hanoi(DISKS, 'A', 'B', 'C');
	door->exit(0);
}
