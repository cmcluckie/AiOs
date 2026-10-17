# PART.screen.vga@1 as built

Hand-derived from the source on 2026-10-16, pending the generator (plan 7.1).

## What it does
Writes text to the 80 by 25 VGA text screen at the cursor, in the current
colour, and scrolls everything up a line when a character arrives with the
cursor past the bottom: Linux's early_vga_write, unchanged but for the colour.
A newline moves to the start of the next line; a carriage return is dropped;
the 81st character of a line wraps.  Starts with the cursor below the last
line, as Linux does, so the first character scrolls; the kernel clears the
screen and sends the cursor home before anything is written.
color_table turns an ANSI colour number into VGA's (ANSI red is 1, VGA red is
4).  Ours: vga_set_color, vga_home, vga_clear.

## Every syscall or hardware access
Memory 0xb8000 to 0xb8f9f, 16 bits at a time, read only to scroll.  No port:
the card's registers are never touched; its own BIOS set the mode.

## Every symbol it imports
None.

## Every byte it writes anywhere
The 2000 text cells, and its own cursor and colour.

## Size
55 lines of code: 40 lifted (early_printk.c 25-29, 31-48, 57-70; vt.c 1399-1400),
15 ours.
