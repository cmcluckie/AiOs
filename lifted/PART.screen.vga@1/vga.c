/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.screen.vga@1: the lines that put text on a PC's screen, and the table
 * that turns an ANSI colour into a VGA one.  Exposes CAP.screen.write.
 * Requires CAP.dev.vga.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   arch/x86/kernel/early_printk.c   "Simple VGA output" 25-29, early_vga_write 31-48, 57-70
 *                                    (the same file PART.serial.8250 came from)
 *   drivers/tty/vt/vt.c              color_table 1399-1400
 *
 * Cut, and why:
 *   __ISA_IO_base + 0xb8000          the kernel runs with the first megabyte mapped where it is
 *   readw, writew (asm/io.h)         replaced by plain volatile 16-bit accesses, which is
 *                                    what they compile to on x86
 *   the CONFIG_KGDB_KDB block        backspace and carriage return for the kernel debugger;
 *                                    without it '\r' is dropped, as Linux does
 *   struct console early_vga_console the console layer
 *   the console argument             one screen
 * Changed, and why:
 *   (0x7 << 8)                       the colour of every character Linux writes, fixed at
 *                                    light grey on black.  Here it is vga_color, so a
 *                                    program's colours reach the screen (findings 0019 #2)
 *
 * Ours, below the line: vga_color, vga_set_color, vga_home and vga_clear, the
 * glue the kernel's console needs to follow "clear the screen", "go to the top"
 * and "change colour".
 */

#include "vga.h"

#define readw(a)	(*(volatile unsigned short *)(a))
#define writew(v, a)	(*(volatile unsigned short *)(a) = (v))

static unsigned short vga_color = 0x7;	/* ours: Linux's fixed light grey on black, until told otherwise */

/* arch/x86/kernel/early_printk.c 25-29 */
/* Simple VGA output */
#define VGABASE		(0xb8000)

static int max_ypos = 25, max_xpos = 80;
static int current_ypos = 25, current_xpos;

/* arch/x86/kernel/early_printk.c 31-48, 57-70 */
void vga_write(const char *str, unsigned n)
{
	char c;
	int  i, k, j;

	while ((c = *str++) != '\0' && n-- > 0) {
		if (current_ypos >= max_ypos) {
			/* scroll 1 line up */
			for (k = 1, j = 0; k < max_ypos; k++, j++) {
				for (i = 0; i < max_xpos; i++) {
					writew(readw(VGABASE+2*(max_xpos*k+i)),
					       VGABASE + 2*(max_xpos*j + i));
				}
			}
			for (i = 0; i < max_xpos; i++)
				writew(0x720, VGABASE + 2*(max_xpos*j + i));
			current_ypos = max_ypos-1;
		}
		if (c == '\n') {
			current_xpos = 0;
			current_ypos++;
		} else if (c != '\r')  {
			writew(((vga_color << 8) | (unsigned short) c),
			       VGABASE + 2*(max_xpos*current_ypos +
						current_xpos++));
			if (current_xpos >= max_xpos) {
				current_xpos = 0;
				current_ypos++;
			}
		}
	}
}

/* drivers/tty/vt/vt.c 1399-1400 */
const unsigned char color_table[] = { 0, 4, 2, 6, 1, 5, 3, 7,
				       8,12,10,14, 9,13,11,15 };

/* ---- ours ---- */

void vga_set_color(unsigned char attr)
{
	vga_color = attr;
}

void vga_home(void)
{
	current_xpos = 0;
	current_ypos = 0;
}

void vga_clear(void)
{
	int i;

	for (i = 0; i < max_xpos * max_ypos; i++)
		writew((unsigned short)(vga_color << 8 | ' '), VGABASE + 2*i);
	vga_home();
}
