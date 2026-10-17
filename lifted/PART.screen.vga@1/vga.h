/* PART.screen.vga@1: the 80 by 25 text screen.  Exposes CAP.screen.write. */
#ifndef AIOS_VGA_H
#define AIOS_VGA_H

void vga_write(const char *str, unsigned n);	/* Linux's early_vga_write: text at the cursor, scrolling at the bottom */
void vga_clear(void);				/* every cell blank in the current colour, cursor top left */
void vga_home(void);				/* cursor top left */
void vga_set_color(unsigned char attr);		/* the VGA attribute later text is written in: background << 4 | foreground */
extern const unsigned char color_table[16];	/* ANSI colour number -> VGA colour number */

#endif
