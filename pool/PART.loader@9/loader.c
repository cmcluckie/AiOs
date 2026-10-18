/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.loader@9: the kernel behind the gate (plan 9.13).  It is @8 with the
 * program run in ring 3 by PART.gate@1: the program reaches the kernel only
 * through int 0x80, and the processor stops it if it touches the kernel's
 * memory, the screen's memory or an I/O port.  The system call table a
 * program is handed is the same seven entries as before (door.h), so earlier
 * programs run unchanged; each entry is a stub that does the int 0x80.
 *
 * Several programs at once are PART.share@2, on the gate; a program on its own
 * is run by the gate directly.
 *
 * Everything below is as @8: one kernel source, every part behind a switch
 * the builder sets from the flavor's petitions.
 *
 * Always here, because a kernel without them is not one: the serial console
 * (PART.serial.8250@2), alloc, exit, and the system call table.  Each switch
 * is HAVE_ and the part's name without its version, set by the builder:
 *
 *   HAVE_TIMER_KVMCLOCK   PART.timer.kvmclock@1   the hypervisor says how fast the counter ticks
 *   HAVE_TIMER_TSC_PIT    PART.timer.tsc-pit@N    the counter timed against the 8254
 *   HAVE_FS_MINIX         PART.fs.minix@N         load and save, on PART.disk.ata-pio@1
 *   HAVE_SHARE            PART.share@2          more than one program at once, behind the gate
 *   HAVE_SCREEN_VGA       PART.screen.vga@1       the screen, a second console
 *
 * With both clocks the kernel asks the hypervisor first and times the 8254 if
 * there is no answer, as @7 did.  That is a choice made at run time between
 * two parts, which plan 5.8 says only a petition-requested chooser may make
 * (Linux's clock-source selection, lifted); it is not lifted yet, and the
 * order is still ours (findings 0018 #2).
 *
 * Every console gets every byte written, the way Linux's printk goes to every
 * console it has: the serial port always, and the screen when a petition names
 * it.  The screen follows the few ANSI escapes a program may use for colour
 * (ESC [ ... m), "home" (ESC [ H) and "clear" (ESC [ 2 J); the serial port is
 * handed the escapes as they are, and a terminal on the other end follows them
 * itself.  Anything else after an ESC is dropped from the screen.
 *
 * A system call whose part is absent answers the way it does when the hardware
 * is missing: ms says 0 forever, load and save say -1.  The table itself never
 * changes shape (door.h).
 *
 * Exposes CAP.proc.exit and CAP.mem.alloc.  Everything else a program may do
 * is exposed by the part that does it.
 */

#include "kernel.h"
#include "door.h"
#include "serial8250.h"
#ifdef HAVE_TIMER_TSC_PIT
#include "tsccal.h"
#endif
#ifdef HAVE_TIMER_KVMCLOCK
#include "kvmclock.h"
#endif
#ifdef HAVE_FS_MINIX
#include "minix.h"
#endif
#ifdef HAVE_SHARE
#include "share.h"
#endif
#include "gate.h"
#ifdef HAVE_SCREEN_VGA
#include "vga.h"
#endif

#define CONSOLE_PORT	0x3f8		/* CAP.dev.serial */

/* the heap of the program */
static struct heap plain;

static struct heap *heap(void)
{
#ifdef HAVE_SHARE
	struct heap *h = share_heap();

	if (h)
		return h;
#endif
	return &plain;
}

/* Interrupts off for the length of a system call, so a tick never lands in the
 * middle of one: one program's bytes never interleave with another's. */
static unsigned long irq_save(void)
{
	unsigned long f;

	__asm__ volatile("pushfl; popl %0; cli" : "=r"(f) : : "memory");
	return f;
}

static void irq_restore(unsigned long f)
{
	__asm__ volatile("pushl %0; popfl" : : "r"(f) : "memory", "cc");
}

/*
 * The heap of a plain program: from its end to the end of the RAM region it
 * lives in, from the firmware's memory map, as in @2.
 */
static void heap_init(const struct hvm_start_info *si, unsigned long program_end)
{
	const struct hvm_memmap_table_entry *m = (const void *)(unsigned long)si->memmap_paddr;
	unsigned int i;

	plain.next = (program_end + 15) & ~15UL;
	plain.end = plain.next;	/* no map, no memory: alloc refuses everything */
	if (si->version < 1 || !si->memmap_entries)
		return;
	for (i = 0; i < si->memmap_entries; i++) {
		unsigned long lo = (unsigned long)m[i].addr;
		unsigned long hi = (unsigned long)(m[i].addr + m[i].size);

		if (m[i].type == MEMMAP_RAM && lo <= PROGRAM_BASE && PROGRAM_BASE < hi) {
			plain.end = hi;
			break;
		}
	}
}

/* Is [lo, hi) all RAM, by the firmware's map? */
int kernel_is_ram(const struct hvm_start_info *si, unsigned long lo, unsigned long hi)
{
	const struct hvm_memmap_table_entry *m = (const void *)(unsigned long)si->memmap_paddr;
	unsigned int i;

	for (i = 0; si->version >= 1 && i < si->memmap_entries; i++)
		if (m[i].type == MEMMAP_RAM && m[i].addr <= lo && hi <= m[i].addr + m[i].size)
			return 1;
	return 0;
}

static void *door_alloc(unsigned long bytes)
{
	unsigned long f = irq_save();
	struct heap *t = heap();
	unsigned long p = t->next, n = (bytes + 15) & ~15UL;
	void *r = 0;

	if (!(bytes == 0 || n < bytes || n > t->end - p)) {
		t->next = p + n;
		r = (void *)p;
	}
	irq_restore(f);
	return r;		/* 0: refused; the program decides what that means */
}

/*
 * Halt.  The machine has no power-off device and no petition asks for one, so
 * we ask for a reset the way a bare CPU can: a null IDT and an interrupt is a
 * triple fault, the hypervisor sees a reset, and the machine (-no-reboot)
 * exits instead.  Findings 0002 #6.
 */
void __attribute__((noreturn)) kernel_halt(void)
{
	static const struct { unsigned short limit; unsigned int base; } __attribute__((packed)) null_idt = { 0, 0 };

	__asm__ volatile("cli; lidt %0; int3" : : "m"(null_idt));
	for (;;)
		__asm__ volatile("cli; hlt");
}

#ifdef HAVE_SCREEN_VGA
/*
 * The screen's half of the console: text straight to the screen part, and the
 * escape sequences turned into its colour, home and clear.  An SGR parameter
 * is 0 (light grey on black), 1 (bright), 30-37 (foreground) or 40-47
 * (background), the ANSI colour numbers, which the screen part's table (from
 * Linux's vt.c) turns into VGA's.  Ours.
 */
static unsigned char con_attr = 0x07, con_state, con_np;
static unsigned int con_par[8];
static unsigned int con_col, con_wrapped;	/* where the screen's cursor is in its line, and whether the last character filled it */

static void con_sgr(unsigned int p)
{
	if (p == 0)
		con_attr = 0x07;
	else if (p == 1)
		con_attr |= 0x08;
	else if (p >= 30 && p <= 37)
		con_attr = (unsigned char)((con_attr & 0xf8) | color_table[p - 30]);
	else if (p >= 40 && p <= 47)
		con_attr = (unsigned char)((con_attr & 0x8f) | (color_table[p - 40] << 4));
	vga_set_color(con_attr);
}

/*
 * A newline straight after the 80th character of a line ends that line; it
 * does not start a blank one.  Every terminal since DEC's VT100 (1978) waits
 * like that before wrapping; Linux's early VGA code wraps at once, so on its
 * own a full 80-column line and its newline came out double-spaced (findings
 * 0019 #3).  Ours.
 */
static void screen_text(const char *s, unsigned long n)
{
	unsigned long i, from = 0;

	for (i = 0; i < n; i++) {
		if (s[i] == '\n') {
			if (con_wrapped) {
				if (i > from)
					vga_write(s + from, (unsigned)(i - from));
				from = i + 1;	/* the line already ended at the edge */
			}
			con_col = 0;
			con_wrapped = 0;
		} else if (s[i] != '\r') {
			con_wrapped = 0;
			if (++con_col == 80) {
				con_col = 0;
				con_wrapped = 1;
			}
		}
	}
	if (n > from)
		vga_write(s + from, (unsigned)(n - from));
}

static void screen_put(const char *s, unsigned long n)
{
	unsigned long i, from = 0;
	unsigned int k;

	for (i = 0; i < n; i++) {
		unsigned char c = (unsigned char)s[i];

		if (con_state == 0) {
			if (c != 0x1b)
				continue;
			if (i > from)
				screen_text(s + from, i - from);
			con_state = 1;
		} else if (con_state == 1) {
			con_state = c == '[' ? 2 : 0;
			con_np = 0;
			con_par[0] = 0;
		} else if (c >= '0' && c <= '9') {
			con_par[con_np] = con_par[con_np] * 10 + (c - '0');
		} else if (c == ';') {
			if (con_np < 7)
				con_par[++con_np] = 0;
		} else {
			if (c == 'm')
				for (k = 0; k <= con_np; k++)
					con_sgr(con_par[k]);
			else if (c == 'H') {
				vga_home();
				con_col = con_wrapped = 0;
			} else if (c == 'J' && con_par[0] == 2) {
				vga_clear();
				con_col = con_wrapped = 0;
			}
			con_state = 0;
		}
		if (con_state == 0)
			from = i + 1;
	}
	if (con_state == 0 && n > from)
		screen_text(s + from, n - from);
}
#endif

/* Every byte to every console. */
void kernel_say(const char *s, unsigned long n)
{
	serial8250_write(CONSOLE_PORT, s, n);
#ifdef HAVE_SCREEN_VGA
	screen_put(s, n);
#endif
}

void kernel_flush(void)
{
	serial8250_flush(CONSOLE_PORT);
}

static void door_write(const void *s, unsigned long n)
{
	unsigned long f = irq_save();

	kernel_say(s, n);
	irq_restore(f);
}

/* Wait for one byte, then take whatever else has already arrived.  TECH-DEBT 2:
 * a system call runs with interrupts off, so while a program waits here no
 * other program gets a turn.  It goes in the mechanic, after the series. */
static unsigned long door_read(void *buf, unsigned long n)
{
	unsigned char *p = buf;
	unsigned long got = 0, f;

	if (n == 0)
		return 0;
	while (!serial8250_poll(CONSOLE_PORT))
		;
	f = irq_save();
	p[got++] = serial8250_getc(CONSOLE_PORT);
	while (got < n && serial8250_poll(CONSOLE_PORT))
		p[got++] = serial8250_getc(CONSOLE_PORT);
	irq_restore(f);
	return got;
}

/*
 * The clock, as @4.  khz is how many counter ticks make a millisecond,
 * measured once at boot; start is the counter then.  0 without a clock part.
 */
#if defined(HAVE_TIMER_KVMCLOCK) || defined(HAVE_TIMER_TSC_PIT)
static unsigned long clock_khz;
static unsigned long long clock_start;

/* the processor's counter, read the way Linux's rdtsc() reads it */
static unsigned long long cycles(void)
{
	unsigned long lo, hi;

	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long)hi << 32) | lo;
}
#endif

static unsigned long door_ms(void)
{
#if defined(HAVE_TIMER_KVMCLOCK) || defined(HAVE_TIMER_TSC_PIT)
	unsigned long long t;
	unsigned long hi, lo, q, r;

	if (!clock_khz)
		return 0;
	t = cycles() - clock_start;
	hi = (unsigned long)(t >> 32);
	lo = (unsigned long)t;
	if (hi >= clock_khz)
		return 0xffffffffUL;	/* 49 days and 17 hours: the call's last value, held */
	__asm__("divl %4" : "=a"(q), "=d"(r) : "0"(lo), "1"(hi), "rm"(clock_khz));
	(void)r;
	return q;
#else
	return 0;
#endif
}

static long door_load(const char *name, void *buf, unsigned long max)
{
#ifdef HAVE_FS_MINIX
	unsigned long f = irq_save();
	long r = minix_load(name, buf, max);

	irq_restore(f);
	return r;
#else
	(void)name; (void)buf; (void)max;
	return -1;
#endif
}

static long door_save(const char *name, const void *buf, unsigned long n)
{
#ifdef HAVE_FS_MINIX
	unsigned long f = irq_save();
	long r = minix_save(name, buf, n);

	irq_restore(f);
	return r;
#else
	(void)name; (void)buf; (void)n;
	return -1;
#endif
}

static void __attribute__((noreturn)) door_exit(int code)
{
	(void)code;			/* unobservable from outside the machine; findings 0000 #9 */
#ifdef HAVE_SHARE
	share_exit();			/* resumes the next program, if there is one */
#endif
	__asm__ volatile("cli");
	kernel_flush();
	kernel_halt();
}

/* what the kernel does for each system call; the gate (PART.gate@1) is the only caller */
static const struct door kernel_calls = { door_write, door_exit, door_alloc, door_read, door_ms, door_load, door_save };

void __attribute__((noreturn)) loader_main(const struct hvm_start_info *si)
{
	const struct hvm_modlist_entry *mod;
	const unsigned char *src;
	unsigned char *dst;
	unsigned long n;

	serial8250_init(CONSOLE_PORT);
#ifdef HAVE_SCREEN_VGA
	vga_clear();		/* whatever the firmware left on the screen goes */
#endif
#ifdef HAVE_TIMER_KVMCLOCK
	clock_khz = kvmclock_khz();	/* KVM's word first, as Linux does under KVM; 0 if there is no KVM (findings 0012) */
#endif
#ifdef HAVE_TIMER_TSC_PIT
	{	/* Up to 150 ms against the 8254 per try (findings 0012), then never again.  Findings 0008. */
		int tries;

		for (tries = 0; tries < 5 && !clock_khz; tries++)
			clock_khz = tsccal_khz();
	}
#endif
#if defined(HAVE_TIMER_KVMCLOCK) || defined(HAVE_TIMER_TSC_PIT)
	clock_start = cycles();
#endif

	if (si->magic != XEN_HVM_START_MAGIC_VALUE || si->nr_modules < 1)
		kernel_halt();	/* no program: nothing to say, nothing to do */

	mod = (const struct hvm_modlist_entry *)(unsigned long)si->modlist_paddr;
	src = (const unsigned char *)(unsigned long)mod[0].paddr;

	/* An archive of programs ("070701", cpio's new ASCII header): share, if a
	 * petition asked to.  If none did, nothing here can run more than one
	 * program, and the machine stops without a word: the referee refuses
	 * such a flavor before it is built (rule 19). */
	if (mod[0].size > 6 && src[0] == '0' && src[1] == '7' && src[2] == '0' &&
	    src[3] == '7' && src[4] == '0' && src[5] == '1') {
#ifdef HAVE_SHARE
		share(si, (void *)src, (unsigned long)mod[0].size, &kernel_calls);
#else
		kernel_halt();
#endif
	}

	/* One plain program, as @5: copy it to PROGRAM_BASE.  Then the gate runs it in ring 3. */
	dst = (unsigned char *)PROGRAM_BASE;
	for (n = (unsigned long)mod[0].size; n; n--)
		*dst++ = *src++;
	heap_init(si, (unsigned long)dst);

	gate_run(PROGRAM_BASE, PROGRAM_BASE, plain.end, &kernel_calls);
}
