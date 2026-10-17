/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.life@2.  PUR.0009: Conway's Game of Life on a screen, in colour.
 *
 * The same universe as life@1: an R-pentomino in the middle of 256 x 96 cells
 * with dead edges, 200 generations.  This time it is drawn to be looked at.
 * It clears the screen, writes the generation and the population on the top
 * line, then the middle 80 x 24 cells, one row per line, in colour: a cell
 * born in the last generation bright yellow, a cell that survived it green, an
 * empty cell a dark blue dot.  Then it waits for a key, so a person can look,
 * and exits.  The colours are the ANSI escape codes every terminal knows; the
 * program does not know or care which console draws them.  Requires
 * CAP.tty.write, CAP.tty.read, CAP.proc.exit and CAP.mem.alloc.
 */
#include "../PART.loader@8/door.h"

#define W 256
#define H 96
#define GENERATIONS 200
#define VIEW_W 80
#define VIEW_H 24

#define BORN	"\033[1;33m"	/* bright yellow */
#define SURVIVED "\033[0;32m"	/* green */
#define EMPTY	"\033[0;34m"	/* blue */

static const struct door *door;

static void put(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
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

static unsigned char *grid, *prev;

static int alive(const unsigned char *g, int x, int y)
{
	return x >= 0 && x < W && y >= 0 && y < H && g[y * W + x];
}

/* one generation: prev becomes the old grid, grid the new one */
static void step(void)
{
	int x, y, n;
	unsigned char *t;

	for (y = 0; y < H; y++)
		for (x = 0; x < W; x++) {
			n = alive(grid, x - 1, y - 1) + alive(grid, x, y - 1) + alive(grid, x + 1, y - 1)
			  + alive(grid, x - 1, y)                            + alive(grid, x + 1, y)
			  + alive(grid, x - 1, y + 1) + alive(grid, x, y + 1) + alive(grid, x + 1, y + 1);
			prev[y * W + x] = (unsigned char)(n == 3 || (n == 2 && grid[y * W + x]));
		}
	t = grid; grid = prev; prev = t;
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *d)
{
	unsigned long i, population = 0;
	int x, y, g;
	const char *colour, *last = 0;
	char key;

	door = d;
	grid = door->alloc(W * H);
	prev = door->alloc(W * H);
	if (!grid || !prev) { put("alloc refused\n"); door->exit(1); }
	for (i = 0; i < W * H; i++)
		grid[i] = prev[i] = 0;
	/* R-pentomino, centred:   .##
	 *                          ##.
	 *                          .#.  */
	x = W / 2; y = H / 2;
	grid[(y - 1) * W + x] = grid[(y - 1) * W + x + 1] = 1;
	grid[y * W + x - 1] = grid[y * W + x] = 1;
	grid[(y + 1) * W + x] = 1;
	for (g = 0; g < GENERATIONS; g++)
		step();
	for (i = 0; i < W * H; i++)
		population += grid[i];

	put("\033[0m\033[2J\033[H");
	put("generation "); put_num(GENERATIONS); put(", population "); put_num(population); put("\n");
	for (y = (H - VIEW_H) / 2; y < (H - VIEW_H) / 2 + VIEW_H; y++) {
		for (x = (W - VIEW_W) / 2; x < (W - VIEW_W) / 2 + VIEW_W; x++) {
			if (!grid[y * W + x])
				colour = EMPTY;
			else
				colour = prev[y * W + x] ? SURVIVED : BORN;
			if (colour != last)
				put(colour);
			last = colour;
			door->write(grid[y * W + x] ? "#" : ".", 1);
		}
		put("\n");
	}
	put("\033[0m");
	door->read(&key, 1);
	door->exit(0);
}
