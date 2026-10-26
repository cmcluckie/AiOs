/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.loader@10: @9, and a normal Linux program file (part 20, findings 0023
 * item 10).  When the program the machine hands over is a 32-bit ELF file,
 * static and not position-independent, the kernel puts each loadable piece where
 * the file says, zeroes the rest, lays out the starting stack the way Linux does
 * for a new program (the arguments from the kernel command line, an empty
 * environment, and the facts a C library asks for), and starts it through the
 * gate's gate_exec (PART.gate@2), which answers the calls such a program makes.
 * Everything else is @9, unchanged; what follows is @9's header.
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
 * A part can do more than a flavor asks of it.  For each capability the
 * flavor's petitions ask for, the builder also sets GRANT_ and the
 * capability's name (CAP.fs.write@2 is GRANT_FS_WRITE).  Without
 * GRANT_FS_WRITE, PART.fs.minix@2 compiles in reading only, and save answers
 * -1, as it does with no disk (decision 0065).
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
#if defined(HAVE_FS_MINIX) && defined(GRANT_FS_WRITE)
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

/*
 * A Linux program file (ELF, 32-bit, x86, static).  Only what is needed to load
 * one: the file's header and its program headers (the ELF specification, the
 * System V ABI; Linux's own reading of them is fs/binfmt_elf.c).
 */
struct elf_hdr {
	unsigned char  ident[16];
	unsigned short type, machine;
	unsigned int   version, entry, phoff, shoff, flags;
	unsigned short ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};
struct elf_phdr {
	unsigned int type, offset, vaddr, paddr, filesz, memsz, flags, align;
};
#define PT_LOAD		1
#define PT_INTERP	3
#define ELF_WINDOW	0x1000000UL	/* 16 MiB: the program, its heap and its stack */
#define ELF_PHYS	0x400000UL	/* the physical RAM the window sits on, from 4 MiB */
#define AT_NULL 0
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_PAGESZ 6
#define AT_ENTRY 9
#define AT_RANDOM 25

static int is_elf(const unsigned char *p, unsigned long n)
{
	return n >= sizeof(struct elf_hdr) && p[0] == 0x7f && p[1] == 'E' && p[2] == 'L' && p[3] == 'F';
}

/* Say why the file was refused, and halt: a program that cannot be loaded never starts. */
static void __attribute__((noreturn)) elf_refuse(const char *why)
{
	unsigned long n = 0;

	while (why[n])
		n++;
	kernel_say("loader: ", 8);
	kernel_say(why, n);
	kernel_say("\n", 1);
	kernel_flush();
	kernel_halt();
}

/*
 * Try rdrand once; return 1 and fill *out on success, 0 on failure.
 * The CF flag is 1 if the hardware gave a valid number (Intel SDM, RDRAND).
 */
static int rdrand32(unsigned int *out)
{
	unsigned int v;
	unsigned char ok;

	__asm__ volatile(".byte 0x0f, 0xc7, 0xf0\n\t"	/* rdrand %eax */
			 "setc %1"
			 : "=a"(v), "=qm"(ok)
			 :
			 : "cc");
	*out = v;
	return (int)ok;
}

/*
 * Does the processor have rdrand?  CPUID leaf 1, ECX bit 30, the way Linux
 * decides X86_FEATURE_RDRAND (arch/x86/kernel/cpu/common.c).
 */
static int has_rdrand(void)
{
	unsigned int eax = 1, ecx = 0;

	__asm__ volatile("cpuid" : "+a"(eax), "+c"(ecx) : : "ebx", "edx");	/* cpuid writes all four */
	return (ecx >> 30) & 1;
}

/*
 * Load and start it.  The window is the 16 MiB from the first piece's 4 MiB
 * boundary, on the physical RAM from 4 MiB (the program's addresses are its own;
 * the gate maps them).  The stack is laid out at the top, as Linux's
 * create_elf_tables does: argc, the argument pointers, a zero, the environment
 * (none), a zero, the facts (auxiliary vector), then the strings and 16 bytes for
 * AT_RANDOM above them.  The 16 bytes come from rdrand where the processor has
 * it (CPUID leaf 1, ECX bit 30), trying up to 10 times per word as Linux does
 * (RDRAND_RETRY_LOOPS, arch/x86/include/asm/archrandom.h); TECH-DEBT 4: where
 * there is no rdrand, or it fails 10 times, the bytes are the processor's cycle
 * counter, not random (decision 0046).
 */
static void __attribute__((noreturn)) elf_run(const struct hvm_start_info *si, const unsigned char *f, unsigned long size)
{
	const struct elf_hdr *h = (const void *)f;
	const struct elf_phdr *ph;
	unsigned long lo = 0, end = 0, i, k, phdr_at = 0, n, argc = 0;
	unsigned long *sp, uargv[16];
	unsigned char *top, *dst;
	const char *cmd = si->cmdline_paddr ? (const char *)(unsigned long)si->cmdline_paddr : "";

	if (h->ident[4] != 1 || h->ident[5] != 1 || h->machine != 3 || h->type != 2)
		elf_refuse("not a 32-bit x86 program file that can run as it is");
	if (h->phentsize != sizeof(struct elf_phdr) || h->phnum == 0 || h->phnum > 16 ||
	    h->phoff > size || h->phnum * sizeof(struct elf_phdr) > size - h->phoff)
		elf_refuse("the program headers are not where the file says");
	ph = (const void *)(f + h->phoff);
	for (i = 0; i < h->phnum; i++) {
		if (ph[i].type == PT_INTERP)
			elf_refuse("it asks for a dynamic linker; only static programs run here");
		if (ph[i].type != PT_LOAD)
			continue;
		if (ph[i].filesz > ph[i].memsz || ph[i].offset > size || ph[i].filesz > size - ph[i].offset)
			elf_refuse("a piece is bigger than the file");
		if (!lo)
			lo = ph[i].vaddr & ~0x3fffffUL;
		if (ph[i].vaddr < lo || ph[i].vaddr + ph[i].memsz > lo + ELF_WINDOW / 2)
			elf_refuse("a piece is outside the program's window");
		if (ph[i].offset <= h->phoff && h->phoff < ph[i].offset + ph[i].filesz)
			phdr_at = ph[i].vaddr + (h->phoff - ph[i].offset);
		if (ph[i].vaddr + ph[i].memsz > end)
			end = ph[i].vaddr + ph[i].memsz;
	}
	if (!lo)
		elf_refuse("nothing in it to load");
	if ((unsigned long)f < ELF_PHYS + ELF_WINDOW && (unsigned long)f + size > ELF_PHYS)
		elf_refuse("the machine put the program file where its window must go");
	if (!kernel_is_ram(si, ELF_PHYS, ELF_PHYS + ELF_WINDOW))
		elf_refuse("the machine has no RAM for the program's window");
	/* every piece where the file says, through the physical address under it; the rest zeroed */
	dst = (unsigned char *)ELF_PHYS;
	for (k = 0; k < ELF_WINDOW; k++)
		dst[k] = 0;
	for (i = 0; i < h->phnum; i++) {
		if (ph[i].type != PT_LOAD)
			continue;
		dst = (unsigned char *)(ELF_PHYS + (ph[i].vaddr - lo));
		for (k = 0; k < ph[i].filesz; k++)
			dst[k] = f[ph[i].offset + k];
	}

	/* the strings first, at the very top: AT_RANDOM's 16 bytes, then the arguments */
	top = (unsigned char *)(ELF_PHYS + ELF_WINDOW);
	top -= 16;
	{
		/*
		 * Fill the 16 AT_RANDOM bytes from rdrand (4 words of 32 bits), asking
		 * up to 10 times per word as Linux does (RDRAND_RETRY_LOOPS).  If the
		 * processor has no rdrand, or a word fails all 10 tries, fall back to
		 * the cycle counter for that word.  TECH-DEBT 4: the cycle counter is
		 * not random (decision 0046); it is the fallback only.
		 */
		int use_rdrand = has_rdrand();
		unsigned int *rnd_words = (unsigned int *)top;
		unsigned int j;

		for (j = 0; j < 4; j++) {
			unsigned int val;
			int got = 0;

			if (use_rdrand) {
				int tries;

				for (tries = 0; tries < 10; tries++) {
					if (rdrand32(&val)) {
						got = 1;
						break;
					}
				}
			}
			if (!got) {
				/* TECH-DEBT 4: fallback to cycle counter, not random (decision 0046) */
				unsigned long tsc_lo, tsc_hi;

				__asm__ volatile("rdtsc" : "=a"(tsc_lo), "=d"(tsc_hi));
				val = (unsigned int)(tsc_lo ^ (tsc_hi << 16) ^ (j * 0x9e3779b9U));
			}
			rnd_words[j] = val;
		}
	}
	{
		unsigned long rnd = lo + ((unsigned long)top - ELF_PHYS);
		const char *c = cmd;
		unsigned long len;

		while (*c && argc < 15) {
			const char *w;

			while (*c == ' ')
				c++;
			if (!*c)
				break;
			w = c;
			while (*c && *c != ' ')
				c++;
			len = (unsigned long)(c - w);
			top -= len + 1;
			for (k = 0; k < len; k++)
				top[k] = (unsigned char)w[k];
			top[len] = 0;
			uargv[argc++] = lo + ((unsigned long)top - ELF_PHYS);
		}
		/* the table, 16-byte aligned, growing down */
		n = 1 + argc + 1 + 1 + 2 * 7;
		sp = (unsigned long *)(((unsigned long)top - n * 4) & ~15UL);
		k = 0;
		sp[k++] = argc;
		for (i = 0; i < argc; i++)
			sp[k++] = uargv[i];
		sp[k++] = 0;				/* end of the arguments */
		sp[k++] = 0;				/* no environment */
		sp[k++] = AT_PHDR;   sp[k++] = phdr_at;
		sp[k++] = AT_PHENT;  sp[k++] = sizeof(struct elf_phdr);
		sp[k++] = AT_PHNUM;  sp[k++] = h->phnum;
		sp[k++] = AT_PAGESZ; sp[k++] = 4096;
		sp[k++] = AT_ENTRY;  sp[k++] = h->entry;
		sp[k++] = AT_RANDOM; sp[k++] = rnd;
		sp[k++] = AT_NULL;   sp[k++] = 0;
	}
	gate_exec(h->entry, ELF_PHYS, lo, ELF_WINDOW, lo + ((unsigned long)sp - ELF_PHYS), (end + 4095) & ~4095UL, &kernel_calls);
}

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

	/* A Linux program file: loaded where it says and started the way Linux starts one. */
	if (is_elf(src, (unsigned long)mod[0].size))
		elf_run(si, src, (unsigned long)mod[0].size);

	/* One plain program, as @5: copy it to PROGRAM_BASE.  Then the gate runs it in ring 3. */
	dst = (unsigned char *)PROGRAM_BASE;
	for (n = (unsigned long)mod[0].size; n; n--)
		*dst++ = *src++;
	heap_init(si, (unsigned long)dst);

	gate_run(PROGRAM_BASE, PROGRAM_BASE, plain.end, &kernel_calls);
}
