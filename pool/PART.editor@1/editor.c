/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.editor@1: Notepad.  One file, notes.txt, on the disk.  Open it, type,
 * save.  No modes, no tabs, no remembering the last session, nothing to learn:
 * every command there is sits on the bottom line the whole time.
 *
 *   typing types; the arrows, Home, End, Backspace, Delete and Enter do what
 *   they say                                                   SPEC.editor.1@1
 *   Ctrl+S writes exactly the text on screen to the file       SPEC.editor.2@1
 *   Ctrl+Z undoes the last run of typing or deleting, and a
 *   second Ctrl+Z puts it back; one level only                 SPEC.editor.3@1
 *   Ctrl+Q quits; with unsaved changes it asks once            SPEC.editor.4@1
 *   the bottom line always shows every command there is        SPEC.editor.5@1
 *
 * The keys are the ones IBM's Common User Access (1987) settled and Notepad
 * kept.  The screen is a VT100's, 80 by 24 (DEC, 1978): the serial line
 * speaks VT100, so the arrows arrive as its escape codes and the screen is
 * drawn with them.  The file is capped at 7 KiB, the most PART.fs.minix@2
 * holds, and the bottom line says how close it is.
 *
 * Uses the write, exit, read, load and save system calls of PART.loader@7,
 * whose save writes over a file that is already there.  Ours.
 */

#include "../PART.loader@7/door.h"

#define MAX	7168		/* seven 1 KiB zones: PART.fs.minix@2's limit */
#define COLS	80
#define ROWS	23		/* text rows; the 24th is the bottom line */

static const char name[] = "notes.txt";
static const struct door *d;

static char text[MAX], snap[MAX];	/* the file, and the one undo */
static unsigned long len, cur, snaplen, snapcur;
static int have_snap;			/* there is something to undo */
static int run;				/* 0 none, 1 typing, 2 deleting: what the snapshot covers */
static int dirty;			/* changed since it was loaded or saved */
static int asked;			/* Ctrl+Q once with unsaved changes */
static const char *msg;			/* one line of news on the bottom line, until the next key */
static unsigned long top, left;		/* the first row and column on screen */

/* ---- the screen: one frame per batch of keys, built here, written once ---- */

static char out[COLS * 32 * 2 + 256];
static unsigned long on;

static void put(const char *s, unsigned long n)
{
	while (n-- && on < sizeof out)
		out[on++] = *s++;
}

static void puts_(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	put(s, n);
}

static void putnum(unsigned long n)
{
	char t[12];
	int k = 0;

	do
		t[k++] = '0' + n % 10;
	while ((n /= 10) != 0);
	while (k)
		put(&t[--k], 1);
}

static unsigned long line_start(unsigned long p)
{
	while (p > 0 && text[p - 1] != '\n')
		p--;
	return p;
}

static unsigned long line_end(unsigned long p)
{
	while (p < len && text[p] != '\n')
		p++;
	return p;
}

static unsigned long row_of(unsigned long p)
{
	unsigned long r = 0, i;

	for (i = 0; i < p; i++)
		if (text[i] == '\n')
			r++;
	return r;
}

static void draw(void)
{
	unsigned long row = row_of(cur), col = cur - line_start(cur), lines = row_of(len) + 1;
	unsigned long r, p, n;
	char st[COLS];
	unsigned long sn = 0;

	if (row < top)
		top = row;
	if (row >= top + ROWS)
		top = row - ROWS + 1;
	if (col < left)
		left = col;
	if (col >= left + COLS)
		left = col - COLS + 1;

	on = 0;
	puts_("\033[?25l\033[H");		/* cursor off while drawing; home */
	for (p = 0, r = 0; r < top; p++)
		if (text[p] == '\n')
			r++;
	for (r = 0; r < ROWS; r++) {
		if (top + r < lines) {
			unsigned long e = line_end(p);

			if (e - p > left) {
				n = e - p - left;
				put(text + p + left, n > COLS ? COLS : n);
			}
			p = e + 1;
		}
		puts_("\033[K\r\n");
	}

	/* the bottom line: every command there is, then the news or the file */
	{
		const char *a = " Ctrl+S Save   Ctrl+Z Undo   Ctrl+Q Quit   ";
		char num[12];
		int k = 0;

		while (*a && sn < COLS - 1)
			st[sn++] = *a++;
		if (msg) {
			for (a = msg; *a && sn < COLS - 1; )
				st[sn++] = *a++;
		} else {
			for (a = name; *a && sn < COLS - 1; )
				st[sn++] = *a++;
			st[sn++] = dirty ? '*' : ' ';
			st[sn++] = ' ';
			st[sn++] = ' ';
			n = len;
			do
				num[k++] = '0' + n % 10;
			while ((n /= 10) != 0);
			while (k)
				st[sn++] = num[--k];
			for (a = " of 7168"; *a; )
				st[sn++] = *a++;
		}
		while (sn < COLS - 1)
			st[sn++] = ' ';
	}
	puts_("\033[7m");
	put(st, sn);
	puts_("\033[0m\033[");
	putnum(row - top + 1);
	put(";", 1);
	putnum(col - left + 1);
	puts_("H\033[?25h");
	d->write(out, on);
}

/* ---- editing ---- */

/* A run is a stretch of typing, or of deleting, with no move in between.
 * The snapshot is the text as it was when the run began. */
static void begin(int kind)
{
	unsigned long i;

	if (run == kind)
		return;
	for (i = 0; i < len; i++)
		snap[i] = text[i];
	snaplen = len;
	snapcur = cur;
	have_snap = 1;
	run = kind;
}

static void insert(char c)
{
	unsigned long i;

	if (len == MAX) {
		msg = "Full: 7168 is the most a file can hold.";
		return;
	}
	begin(1);
	for (i = len; i > cur; i--)
		text[i] = text[i - 1];
	text[cur++] = c;
	len++;
	dirty = 1;
}

static void delete_at(unsigned long p)
{
	unsigned long i;

	if (p >= len)
		return;
	begin(2);
	for (i = p; i + 1 < len; i++)
		text[i] = text[i + 1];
	len--;
	dirty = 1;
}

/* Ctrl+Z: the text and the snapshot change places, so a second Ctrl+Z puts it back */
static void undo(void)
{
	unsigned long i, n = len > snaplen ? len : snaplen, t;
	char c;

	if (!have_snap)
		return;
	for (i = 0; i < n; i++) {
		c = text[i];
		text[i] = snap[i];
		snap[i] = c;
	}
	t = len; len = snaplen; snaplen = t;
	t = cur; cur = snapcur; snapcur = t;
	run = 0;
	dirty = 1;
}

static void move(unsigned long to)
{
	cur = to;
	run = 0;
}

static void up(void)
{
	unsigned long s = line_start(cur), col = cur - s, ps, pl;

	if (s == 0)
		return;
	ps = line_start(s - 1);
	pl = s - 1 - ps;
	move(ps + (col < pl ? col : pl));
}

static void down(void)
{
	unsigned long s = line_start(cur), col = cur - s, e = line_end(cur), ns, nl;

	if (e == len)
		return;
	ns = e + 1;
	nl = line_end(ns) - ns;
	move(ns + (col < nl ? col : nl));
}

static void save(void)
{
	if (d->save(name, text, len) == 0) {
		dirty = 0;
		msg = "Saved.";
	} else {
		msg = "Not saved: the disk said no.";
	}
}

static void quit(void)
{
	static const char bye[] = "\033[0m\033[2J\033[H";

	if (dirty && !asked) {
		asked = 1;
		msg = "Unsaved.  Ctrl+Q again to quit.";
		return;
	}
	d->write(bye, sizeof bye - 1);
	d->exit(0);
}

/* ---- keys: plain bytes, and the VT100's escape codes for the rest ---- */

static int esc;			/* 0 plain, 1 after ESC, 2 inside ESC [ or ESC O */
static unsigned long arg;	/* the number in ESC [ n ~ */
static int was_cr;		/* the last byte was \r: a \n straight after it is the same Enter */

static void key(unsigned char c)
{
	int q = 0;

	if (esc == 1) {
		esc = (c == '[' || c == 'O') ? 2 : 0;
		arg = 0;
		if (esc)
			return;
	} else if (esc == 2) {
		if (c >= '0' && c <= '9') {
			arg = arg * 10 + (c - '0');
			return;
		}
		if (c == ';')
			return;
		esc = 0;
		switch (c) {
		case 'A': up(); break;
		case 'B': down(); break;
		case 'C': if (cur < len) move(cur + 1); break;
		case 'D': if (cur > 0) move(cur - 1); break;
		case 'H': move(line_start(cur)); break;
		case 'F': move(line_end(cur)); break;
		case '~':
			if (arg == 1 || arg == 7)
				move(line_start(cur));
			else if (arg == 4 || arg == 8)
				move(line_end(cur));
			else if (arg == 3)
				delete_at(cur);
			break;
		}
		asked = 0;
		return;
	}
	if (c == '\n' && was_cr) {
		was_cr = 0;
		return;
	}
	was_cr = (c == '\r');
	switch (c) {
	case 0x1b: esc = 1; return;
	case 0x13: save(); break;			/* Ctrl+S */
	case 0x1a: undo(); break;			/* Ctrl+Z */
	case 0x11: quit(); q = 1; break;		/* Ctrl+Q */
	case '\r': case '\n': insert('\n'); break;	/* Enter */
	case 0x7f: case 0x08:				/* Backspace */
		if (cur > 0) {
			cur--;
			delete_at(cur);
		}
		break;
	default:
		if (c >= 0x20 && c < 0x7f)
			insert((char)c);
		break;
	}
	if (!q)
		asked = 0;
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *door)
{
	static unsigned char in[256];
	static const char clear[] = "\033[0m\033[2J";
	long got;

	d = door;
	got = d->load(name, text, MAX);
	len = got > 0 ? (unsigned long)got : 0;
	d->write(clear, sizeof clear - 1);
	draw();
	for (;;) {
		unsigned long n, i;

		n = d->read(in, sizeof in);
		for (i = 0; i < n; i++) {
			key(in[i]);
			if (!esc) {		/* one frame per whole key, so what you see follows every key */
				draw();
				msg = 0;
			}
		}
	}
}
