/* SPDX-License-Identifier: MIT
 *
 * CMP.life@1.  PUR.0002: run Conway's Game of Life from an R-pentomino for 200
 * generations in a 256 x 96 universe, print the middle of it and the population,
 * then ask for a universe bigger than the machine and print that it was refused.
 *
 * Every CS student wrote this once.  Here it is the memory chapter: two grids of
 * 24 KiB each, far more than the 4 KiB stack, so the program has to ask the
 * loader for them through the door.  Requires CAP.tty.write, CAP.proc.exit and
 * CAP.mem.alloc, and nothing else.  Dead cells beyond the edge; no wrap.
 */

#include "../CMP.loader@2/door.h"

#define W 256
#define H 96
#define GENERATIONS 200
#define VIEW_W 80
#define VIEW_H 24

static const struct door *door;

static void put(const char *s)
{
	unsigned long n = 0;
	while (s[n]) n++;
	door->write(s, n);
}

static void put_num(unsigned long v)
{
	char buf[11];
	int i = 10;
	buf[i] = '\0';
	do { buf[--i] = (char)('0' + v % 10); v /= 10; } while (v);
	put(buf + i);
}

static unsigned char *grid, *next;

static int alive(int x, int y)
{
	return x >= 0 && x < W && y >= 0 && y < H && grid[y * W + x];
}

static void step(void)
{
	int x, y, n;
	unsigned char *t;

	for (y = 0; y < H; y++)
		for (x = 0; x < W; x++) {
			n = alive(x - 1, y - 1) + alive(x, y - 1) + alive(x + 1, y - 1)
			  + alive(x - 1, y)                      + alive(x + 1, y)
			  + alive(x - 1, y + 1) + alive(x, y + 1) + alive(x + 1, y + 1);
			next[y * W + x] = (unsigned char)(n == 3 || (n == 2 && grid[y * W + x]));
		}
	t = grid; grid = next; next = t;
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	unsigned long i, population = 0;
	int x, y, g;
	char line[VIEW_W + 2];

	door = d;
	grid = door->alloc(W * H);
	next = door->alloc(W * H);
	if (!grid || !next) { put("alloc refused\n"); door->exit(1); }
	for (i = 0; i < W * H; i++) grid[i] = 0;

	/* R-pentomino, centred:   .##
	 *                          ##.
	 *                          .#.  */
	x = W / 2; y = H / 2;
	grid[(y - 1) * W + x] = grid[(y - 1) * W + x + 1] = 1;
	grid[y * W + x - 1] = grid[y * W + x] = 1;
	grid[(y + 1) * W + x] = 1;

	for (g = 0; g < GENERATIONS; g++)
		step();

	for (i = 0; i < W * H; i++) population += grid[i];
	put("generation "); put_num(GENERATIONS); put(", population "); put_num(population); put("\n");

	line[VIEW_W] = '\n'; line[VIEW_W + 1] = '\0';
	for (y = (H - VIEW_H) / 2; y < (H - VIEW_H) / 2 + VIEW_H; y++) {
		for (x = 0; x < VIEW_W; x++)
			line[x] = grid[y * W + (W - VIEW_W) / 2 + x] ? '#' : '.';
		put(line);
	}

	put(door->alloc(128UL * 1024 * 1024) ? "128 MiB: granted\n" : "128 MiB: refused\n");
	door->exit(0);
}
