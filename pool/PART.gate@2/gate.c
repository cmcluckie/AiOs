/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.gate@2: the gate, and the system calls a normal Linux program makes.
 * It is @1, unchanged for every program @1 ran, plus (part 20, findings 0023
 * item 10): a program loaded from a Linux program file (gate_exec), whose
 * window may sit at any virtual address and maps to its own physical RAM, and
 * the calls Berkeley's cat makes through musl, each at Linux's 386 number and
 * each as small as cat needs: open and close (read-only, the disk's root
 * directory), read and write (the console, or an open file), writev, brk,
 * mmap2, munmap, set_thread_area, set_tid_address, statx, ioctl and
 * exit_group.  Anything a call is asked for that it does not do is refused with
 * Linux's error number, never half done.  Ctrl+D ends what is typed, and a
 * Linux program's exit status is said on the console.
 *
 * What follows is @1's header, still true:
 *
 * Programs run in ring 3, the processor's least
 * privileged level, and the only way they reach the kernel is a trap
 * instruction, int 0x80, with the call's number in eax.  A program that
 * touches anything else (the kernel's memory, the screen's, an I/O port) is
 * stopped by the processor itself, and the kernel says so on the console.
 * Plan 9.13, findings 0021.  Ours; nothing lifted.  The descriptor, page-table
 * and task-state-segment bits are the Intel SDM's; the register convention of
 * int 0x80 and the numbers of write, read and exit are Linux's 32-bit x86 ABI.
 *
 * Memory: every address maps to itself.  The kernel's pages, and every page
 * outside the program's window, are closed to ring 3; only the window, the
 * program's stack and the page of system call stubs (gate_user.S) are open.
 * The kernel obeys read-only pages too (CR0.WP).
 *
 * Several programs (PART.share@2): each gets a space, its own page directory
 * that maps the same virtual window to its own physical RAM, and its own kernel
 * stack, where the processor puts a trap's frame.  The gate saves and resumes
 * the frames; who runs next is decided by the scheduler the sharing part
 * registers (gate_schedule), not here.
 *
 * SSE (findings 0023 item 13): if the processor has FXSR and SSE (CPUID leaf 1
 * EDX bits 24 and 25), the kernel sets CR4.OSFXSR and CR4.OSXMMEXCPT, clears
 * CR0.EM and sets CR0.MP, and keeps a 512-byte FXSAVE area for each program,
 * saving and restoring it on every switch, the way Linux does.  A new program's
 * x87 control word is 0x037F: extended precision, every exception masked, as
 * Linux gives.  Without FXSR and SSE the kernel leaves CR0 and CR4 as they
 * were and does not FXSAVE or FXRSTOR, so it runs as it did before.
 *
 * TECH-DEBT 1: alloc, ms, load and save keep their AiOs-only numbers 1000 to
 * 1003 as a second table for programs written for the old table (findings 0021
 * #5, decision 0074).  A Linux program asking 1000 to 1003 gets -38 (ENOSYS),
 * as Linux answers a number it has no call for.  Paid in this part: the
 * numbers stay, as Linux keeps one table for each kind of program, and only a
 * Linux program reaching them was the debt (findings 0024 item 26).
 */

#include "gate.h"

#define SEL_KCODE	0x08
#define SEL_KDATA	0x10
#define SEL_UCODE	0x18		/* with RPL 3: 0x1b */
#define SEL_UDATA	0x20		/* with RPL 3: 0x23 */
#define SEL_TSS		0x28

#define NR_EXIT		1		/* Linux i386 */
#define NR_READ		3
#define NR_WRITE	4
#define NR_ALLOC	1000		/* TECH-DEBT 1: old table only */
#define NR_MS		1001		/* TECH-DEBT 1: old table only */
#define NR_LOAD		1002		/* TECH-DEBT 1: old table only */
#define NR_SAVE		1003		/* TECH-DEBT 1: old table only */
#define NR_OPEN		5
#define NR_CLOSE	6		/* Linux i386, from here on: the calls a normal Linux program makes */
#define NR_BRK		45
#define NR_IOCTL	54
#define NR_MUNMAP	91
#define NR_WRITEV	146
#define NR_MMAP2	192
#define NR_SET_THREAD_AREA 243
#define NR_EXIT_GROUP	252
#define NR_SET_TID_ADDRESS 258
#define NR_STATX	383

#define ENOENT		2
#define ENOMEM		12
#define EMFILE		24
#define EROFS		30
#define EINVAL		22
#define ENOTTY		25
#define AT_EMPTY_PATH	0x1000
#define MAP_PRIVATE	0x02
#define MAP_ANONYMOUS	0x20
#define SEL_TLS		0x30		/* the one thread-storage segment a program may set, GDT entry 6 */
#define TLS_ENTRY	6
#define PAGE		4096UL
#define NFILES		4		/* files a Linux program may have open at once, from descriptor 3 */
#define FILE_MAX	8192		/* the most a file can hold on the disk as lifted (seven 1 KiB zones), and then some */

#define EFAULT		14
#define EBADF		9
#define ENOSYS		38
#define NAME_MAX_LEN	256		/* the longest file name the kernel will read from a program */

extern char kernel_end[], stack_top[];
extern char gate_user_start[], gate_stack_lo[], gate_stack_hi[];
extern char gate_door[], gate_return[];

#define KSTACK_WORDS	4096		/* 16 KiB of kernel stack for each space */
#define FXSAVE_SIZE	512		/* bytes for FXSAVE/FXRSTOR, 16-byte aligned */

struct gate_space {
	unsigned long cr3;		/* its page directory */
	unsigned long *frame;		/* where its registers were saved, on its kernel stack */
	unsigned long kstack_top;	/* where a trap from ring 3 lands (the TSS's esp0) */
	int user_stack;			/* ring 3 stands on the gate's own stack (one program, no sharing) */
	unsigned char fxsave[FXSAVE_SIZE] __attribute__((aligned(16)));	/* SSE/x87 state, when sse_on */
};

static unsigned long long gdt[7];
static unsigned int tss[26];
static unsigned long long idt[256];
static unsigned long pds[GATE_MAX_SPACES][1024] __attribute__((aligned(4096)));
static unsigned long pt0[1024] __attribute__((aligned(4096)));
static unsigned long kstacks[GATE_MAX_SPACES][KSTACK_WORDS] __attribute__((aligned(16)));
static struct gate_space spaces[GATE_MAX_SPACES];
static int nspaces;
static struct gate_space *cur;
static const struct gate_sched *sched;
static int sse_on;			/* 1 if the processor has FXSR and SSE and we enabled them */

static unsigned long win_lo, win_hi;	/* the same virtual window in every space */
static unsigned long brk_lo, brk_cur, map_lo;	/* a Linux program's heap: brk grows up from brk_lo, mmap2 down from map_lo; 0 for the others */
static const struct door *calls;

/* Is [p, p+n) all inside what ring 3 may touch? */
static int user_ok(unsigned long p, unsigned long n)
{
	unsigned long e = p + n;

	if (n == 0)
		return 1;
	if (e < p)
		return 0;
	if (p >= win_lo && e <= win_hi)
		return 1;
	return cur->user_stack && p >= (unsigned long)gate_stack_lo && e <= (unsigned long)gate_stack_hi;
}

/* Is the NUL-terminated string at p entirely inside what ring 3 may touch? */
static int user_name_ok(const char *s)
{
	unsigned long i;

	for (i = 0; i < NAME_MAX_LEN; i++) {
		if (!user_ok((unsigned long)s + i, 1))
			return 0;
		if (!s[i])
			return 1;
	}
	return 0;
}

/* A Linux program's open files: the disk's root directory, read-only.  Each is
 * read whole when it is opened (the file system as lifted reads whole files) and
 * handed out from there. */
static struct {
	int used;
	unsigned long len, pos;
	unsigned char data[FILE_MAX];
} files[NFILES];

static long file_open(unsigned long path, unsigned long flags)
{
	const char *name = (const char *)path;
	long r;
	int i;

	if (!brk_lo)
		return -ENOSYS;
	if (!user_name_ok(name))
		return -EFAULT;
	if ((flags & 3) != 0 || (flags & 0100))
		return -EROFS;		/* written to, or made: the disk is read-only to a Linux program */
	if (*name == '/')
		name++;
	for (r = 0; name[r]; r++)
		if (name[r] == '/')
			return -ENOENT;	/* only the root directory */
	if (!*name)
		return -ENOENT;
	for (i = 0; i < NFILES && files[i].used; i++)
		;
	if (i == NFILES)
		return -EMFILE;
	r = calls->load(name, files[i].data, FILE_MAX);
	if (r < 0)
		return -ENOENT;
	files[i].used = 1;
	files[i].len = (unsigned long)r > FILE_MAX ? FILE_MAX : (unsigned long)r;
	files[i].pos = 0;
	return 3 + i;
}

static int file_ok(unsigned long fd)
{
	return fd >= 3 && fd < 3 + NFILES && files[fd - 3].used;
}

static long file_read(unsigned long fd, unsigned char *b, unsigned long n)
{
	unsigned long k, left = files[fd - 3].len - files[fd - 3].pos;

	if (n > left)
		n = left;
	for (k = 0; k < n; k++)
		b[k] = files[fd - 3].data[files[fd - 3].pos + k];
	files[fd - 3].pos += n;
	return (long)n;
}

/* The console as a Linux program's standard input: what is typed, up to the first
 * Ctrl+D, which ends the input the way it does at the start of a line on a Unix
 * terminal (its line discipline turns it into a read of nothing).  The Ctrl+D is
 * not handed over: what came before it in the same read is, and the next read
 * is the end.  Anything typed after it in the same read is dropped. */
static int console_eof;

static long console_read(char *b, unsigned long n)
{
	unsigned long got, i;

	if (n == 0)
		return 0;
	if (console_eof) {
		console_eof = 0;
		return 0;
	}
	got = calls->read(b, n);
	for (i = 0; i < got; i++)
		if (b[i] == 0x04) {
			console_eof = i > 0;
			return (long)i;
		}
	return (long)got;
}

/* "exit N" on the console when a Linux program ends: its status, which nothing
 * else outside the machine could see. */
static void say_exit(unsigned long code)
{
	char m[16] = "exit ";
	unsigned long k = 5, v = code & 0xff, d = 100;

	while (d > 1 && v < d)
		d /= 10;
	for (; d; d /= 10)
		m[k++] = (char)('0' + (v / d) % 10);
	m[k++] = '\n';
	calls->write(m, k);
}

/* writev(fd, iov, n): write's pieces, in order.  Only the console, as write. */
static long writev(unsigned long fd, unsigned long iov, unsigned long n)
{
	const unsigned long *v = (const unsigned long *)iov;
	unsigned long i;
	long total = 0;

	if (fd != 1 && fd != 2)
		return -EBADF;
	if (n > 1024 || !user_ok(iov, n * 8))
		return -EINVAL;
	for (i = 0; i < n; i++)
		if (!user_ok(v[2 * i], v[2 * i + 1]))
			return -EFAULT;
	for (i = 0; i < n; i++) {
		calls->write((const void *)v[2 * i], v[2 * i + 1]);
		total += (long)v[2 * i + 1];
	}
	return total;
}

/* brk(p): move the end of the heap to p, if p is between where it started and
 * the lowest mapped piece; either way answer where it is now, as Linux does. */
static unsigned long brk(unsigned long p)
{
	if (brk_lo && p >= brk_lo && p <= map_lo)
		brk_cur = p;
	return brk_cur;
}

/* mmap2: only fresh, private, zeroed memory (what malloc asks for), taken from
 * the top of the heap's room down; anything else is refused. */
static long mmap2(unsigned long addr, unsigned long len, unsigned long flags, unsigned long fd)
{
	unsigned long n = (len + PAGE - 1) & ~(PAGE - 1), i;
	unsigned char *p;

	(void)addr;		/* where it would like it is only a hint, in Linux too */
	if (!brk_lo)
		return -ENOSYS;
	if ((flags & (MAP_PRIVATE | MAP_ANONYMOUS)) != (MAP_PRIVATE | MAP_ANONYMOUS) || (long)fd != -1)
		return -EINVAL;
	if (n == 0 || n < len || n > map_lo - brk_cur)
		return -ENOMEM;
	map_lo -= n;
	p = (unsigned char *)map_lo;
	for (i = 0; i < n; i++)
		p[i] = 0;
	return (long)map_lo;
}

/* set_thread_area(u_info): one segment for the program's thread storage, GDT
 * entry 6, at the base it asks for, 4 GiB long, ring 3, data.  Linux's struct
 * user_desc: entry number (-1: choose), base, limit, flags. */
static long set_thread_area(unsigned long u)
{
	unsigned long *d = (unsigned long *)u, base;

	if (!brk_lo)
		return -ENOSYS;
	if (!user_ok(u, 16))
		return -EFAULT;
	if (d[0] != (unsigned long)-1 && d[0] != TLS_ENTRY)
		return -EINVAL;
	base = d[1];
	gdt[TLS_ENTRY] = 0xffffULL | ((unsigned long long)(base & 0xffffffUL) << 16) | (0xf2ULL << 40) |
		(0xfULL << 48) | (0xcULL << 52) | ((unsigned long long)(base >> 24) << 56);
	d[0] = TLS_ENTRY;
	return 0;
}

/* statx(fd, "", AT_EMPTY_PATH, mask, buf): what the console is, for the three
 * descriptors a program starts with (a character device, no size, a 1 KiB block),
 * or an open file (a regular file, read-only, its size).  Linux's struct statx,
 * 256 bytes; only the fields cat reads are set. */
static long statx(unsigned long fd, unsigned long path, unsigned long flags, unsigned long buf)
{
	unsigned char *b = (unsigned char *)buf;
	unsigned long i;

	if (!brk_lo)
		return -ENOSYS;
	if (!user_name_ok((const char *)path) || !user_ok(buf, 256))
		return -EFAULT;
	if (!(flags & AT_EMPTY_PATH) || ((const char *)path)[0])
		return -ENOSYS;		/* by name: not yet */
	if (fd > 2 && !file_ok(fd))
		return -EBADF;
	for (i = 0; i < 256; i++)
		b[i] = 0;
	*(unsigned int *)(b + 0) = 0x7ff;		/* stx_mask: the basic fields */
	*(unsigned int *)(b + 4) = 1024;		/* stx_blksize */
	*(unsigned int *)(b + 16) = 1;			/* stx_nlink */
	if (fd <= 2) {
		*(unsigned short *)(b + 28) = 0020620;	/* stx_mode: the console, a character device, rw--w---- */
	} else {
		*(unsigned short *)(b + 28) = 0100444;	/* a file, r--r--r-- */
		*(unsigned long *)(b + 40) = files[fd - 3].len;	/* stx_size, low half */
	}
	return 0;
}

/* frame: what trap_common saved (gate_entry.S).  Returns the call's answer. */
static long syscall(const unsigned long *frame)
{
	unsigned long nr = frame[7], a = frame[4], b = frame[6], c = frame[5];	/* eax; ebx, ecx, edx */

	switch (nr) {
	case NR_EXIT:
	case NR_EXIT_GROUP:
		if (brk_lo)
			say_exit(a);		/* a Linux program's status, where a shell would show it */
		calls->exit((int)a);
	case NR_WRITE:
		if (a != 1 && a != 2)
			return -EBADF;
		if (!user_ok(b, c))
			return -EFAULT;
		calls->write((const void *)b, c);
		return (long)c;
	case NR_READ:
		if (brk_lo && file_ok(a)) {
			if (!user_ok(b, c))
				return -EFAULT;
			return file_read(a, (unsigned char *)b, c);
		}
		if (a != 0)
			return -EBADF;
		if (!user_ok(b, c))
			return -EFAULT;
		if (brk_lo)
			return console_read((char *)b, c);
		return (long)calls->read((void *)b, c);
	case NR_ALLOC:
		if (brk_lo)
			return -ENOSYS;		/* TECH-DEBT 1: old table only; a Linux program gets -38 */
		return (long)calls->alloc(a);
	case NR_MS:
		if (brk_lo)
			return -ENOSYS;		/* TECH-DEBT 1: old table only; a Linux program gets -38 */
		return (long)calls->ms();
	case NR_LOAD:
		if (brk_lo)
			return -ENOSYS;		/* TECH-DEBT 1: old table only; a Linux program gets -38 */
		if (!user_name_ok((const char *)a) || !user_ok(b, c))
			return -EFAULT;
		return calls->load((const char *)a, (void *)b, c);
	case NR_SAVE:
		if (brk_lo)
			return -ENOSYS;		/* TECH-DEBT 1: old table only; a Linux program gets -38 */
		if (!user_name_ok((const char *)a) || !user_ok(b, c))
			return -EFAULT;
		return calls->save((const char *)a, (const void *)b, c);
	case NR_OPEN:
		return file_open(a, b);
	case NR_CLOSE:
		if (brk_lo && file_ok(a)) {
			files[a - 3].used = 0;
			return 0;
		}
		return a <= 2 ? 0 : -EBADF;
	case NR_IOCTL:
		return a <= 2 || (brk_lo && file_ok(a)) ? -ENOTTY : -EBADF;	/* neither the console nor a file is a terminal the program may steer */
	case NR_WRITEV:
		return writev(a, b, c);
	case NR_BRK:
		return (long)brk(a);
	case NR_MMAP2:
		return mmap2(a, b, frame[1], frame[0]);	/* addr, length; flags in esi, fd in edi (frame: edi esi ebp esp ebx edx ecx eax) */
	case NR_MUNMAP:
		return brk_lo ? 0 : -ENOSYS;	/* nothing given back: the memory stays the program's until it exits */
	case NR_SET_THREAD_AREA:
		return set_thread_area(a);
	case NR_SET_TID_ADDRESS:
		return brk_lo ? 1 : -ENOSYS;	/* one program, one thread: its number is 1 */
	case NR_STATX:
		return statx(a, b, c, frame[0]);	/* dirfd, path, flags; the mask in esi is ignored, the answer goes to edi's address */
	}
	return -ENOSYS;
}

static void hex8(char *p, unsigned long v)
{
	int i;

	for (i = 7; i >= 0; i--, v >>= 4)
		p[i] = "0123456789ABCDEF"[v & 15];
}

/*
 * Every trap.  frame points at what trap_common saved: eight registers (popal
 * order, edi first), es, ds, the vector, the error code, then what the
 * processor pushed.  Returns the frame to resume.
 */
unsigned long *gate_trap(unsigned long *frame)
{
	unsigned long vec = frame[10], addr;
	struct gate_space *next = cur;
	static char msg[] = "\nGuru Meditation #XXXXXXXX.XXXXXXXX  program 1 stopped\n";

	cur->frame = frame;
	if (vec == 0x80) {
		if (sse_on)
			__asm__ volatile("fxsave %0" : "=m"(cur->fxsave));
		frame[7] = (unsigned long)syscall(frame);
		if (sse_on)
			__asm__ volatile("fxrstor %0" : : "m"(cur->fxsave));
		return frame;
	}
	if (sched && ((vec & ~0xfUL) == 0x30 || vec == 0xff)) {
		/* a hardware interrupt: the scheduler may take the processor away */
		if (sse_on)
			__asm__ volatile("fxsave %0" : "=m"(cur->fxsave));
		next = sched->irq(vec, cur);
	} else {
		/* The processor refused something.  The two numbers are the vector and the
		 * address: the one it touched (page faults) or the instruction it was on. */
		if (vec == 14)
			__asm__ volatile("movl %%cr2, %0" : "=r"(addr));
		else
			addr = frame[12];
		if (sched && (frame[13] & 3) == 3) {
			if (sse_on)
				__asm__ volatile("fxsave %0" : "=m"(cur->fxsave));
			next = sched->fault(vec, addr, cur);
		} else {
			hex8(msg + 18, vec);
			hex8(msg + 27, addr);
			kernel_say(msg, sizeof msg - 1);
			kernel_flush();
			kernel_halt();
		}
	}
	if (next != cur) {
		cur = next;
		tss[1] = (unsigned int)next->kstack_top;
		__asm__ volatile("movl %0, %%cr3" : : "r"(next->cr3) : "memory");
	}
	if (sse_on)
		__asm__ volatile("fxrstor %0" : : "m"(next->fxsave));
	return next->frame;
}

static void set_gate(int v, const char *handler, unsigned int dpl)
{
	unsigned long h = (unsigned long)handler;

	/* present, 32-bit interrupt gate (interrupts off on entry), code selector 0x08 */
	idt[v] = (h & 0xffff) | ((unsigned long long)SEL_KCODE << 16) |
		((unsigned long long)(0x8e00 | (dpl << 13) | (h & 0xffff0000)) << 32);
}

static void tables_init(void)
{
	static struct { unsigned short limit; unsigned int base; } __attribute__((packed)) gd, id;
	extern char trap0[], trap1[], trap2[], trap3[], trap4[], trap5[], trap6[], trap7[],
		trap8[], trap9[], trap10[], trap11[], trap12[], trap13[], trap14[], trap15[],
		trap16[], trap17[], trap18[], trap19[], trap20[], trap21[], trap22[], trap23[],
		trap24[], trap25[], trap26[], trap27[], trap28[], trap29[], trap30[], trap31[],
		trap0x80[];
	char *t[32] = { trap0, trap1, trap2, trap3, trap4, trap5, trap6, trap7,
		trap8, trap9, trap10, trap11, trap12, trap13, trap14, trap15,
		trap16, trap17, trap18, trap19, trap20, trap21, trap22, trap23,
		trap24, trap25, trap26, trap27, trap28, trap29, trap30, trap31 };
	unsigned long base = (unsigned long)tss, limit = sizeof tss - 1;
	int i;

	gdt[0] = 0;
	gdt[1] = 0x00cf9a000000ffffULL;		/* kernel code: base 0, 4 GiB, ring 0 */
	gdt[2] = 0x00cf92000000ffffULL;		/* kernel data */
	gdt[3] = 0x00cffa000000ffffULL;		/* user code: the same, ring 3 */
	gdt[4] = 0x00cff2000000ffffULL;		/* user data */
	gdt[5] = (limit & 0xffff) | ((unsigned long long)(base & 0xffffffUL) << 16) | (0x89ULL << 40) |
		((unsigned long long)(limit >> 16) << 48) | ((unsigned long long)(base >> 24) << 56);

	tss[1] = (unsigned int)stack_top;	/* esp0: where a trap from ring 3 lands */
	tss[2] = SEL_KDATA;			/* ss0 */
	tss[25] = sizeof tss << 16;		/* no I/O bitmap: every port is closed to ring 3 */

	gd.limit = sizeof gdt - 1;
	gd.base = (unsigned int)gdt;
	__asm__ volatile("lgdt %0; movl %1, %%eax; movl %%eax, %%ds; movl %%eax, %%es; movl %%eax, %%ss; "
		"movl %%eax, %%fs; movl %%eax, %%gs; ljmp %2, $1f; 1:"
		: : "m"(gd), "i"(SEL_KDATA), "i"(SEL_KCODE) : "eax", "memory");
	__asm__ volatile("ltr %%ax" : : "a"(SEL_TSS));

	for (i = 0; i < 32; i++)
		set_gate(i, t[i], 0);
	set_gate(0x80, trap0x80, 3);		/* the one gate a program may knock on */
	if (sched) {
		/* hardware interrupts: the 8259's sixteen vectors from 0x30, and the APIC's spurious one */
		extern char *trap_irq_table[17];
		for (i = 0; i < 16; i++)
			set_gate(0x30 + i, trap_irq_table[i], 0);
		set_gate(0xff, trap_irq_table[16], 0);
	}
	id.limit = sizeof idt - 1;
	id.base = (unsigned int)idt;
	__asm__ volatile("lidt %0" : : "m"(id));
}

/* The first 4 MiB, page by page: the kernel, closed to ring 3, but for the stub page. */
static void base_init(int user_stack)
{
	unsigned long i, stub = (unsigned long)gate_user_start >> 12;

	for (i = 0; i < 1024; i++)
		pt0[i] = (i << 12) | 0x3;	/* present, writable, kernel only */
	pt0[stub] = (stub << 12) | 0x5;		/* present, read-only, ring 3 may read and run it */
	if (user_stack)
		for (i = (unsigned long)gate_stack_lo >> 12; i < (unsigned long)gate_stack_hi >> 12; i++)
			pt0[i] |= 0x4;		/* ring 3's stack, when it stands on the gate's */
}

/* A page directory: every address maps to itself, closed to ring 3, except that
 * the window [virt, virt + len) maps to physical memory from PHYS and is open.
 * TECH-DEBT 3: the window is rounded up to whole 4 MiB pages, so a program can
 * reach the tail past its end.  It goes in the mechanic, after the series. */
static void space_map(unsigned long *pd, unsigned long phys, unsigned long virt, unsigned long len)
{
	unsigned long i, n = (len + 0x3fffffUL) >> 22;

	pd[0] = (unsigned long)pt0 | 0x7;	/* the first 4 MiB: page by page */
	for (i = 1; i < 1024; i++)
		pd[i] = (i << 22) | 0x83;	/* 4 MiB pages, present, writable, kernel only */
	for (i = 0; i < n; i++)
		pd[(virt >> 22) + i] = (phys + (i << 22)) | 0x87;	/* the program's window: ring 3 may use it */
}

static void paging_on(unsigned long cr3)
{
	__asm__ volatile("movl %%cr4, %%eax; orl $0x10, %%eax; movl %%eax, %%cr4" : : : "eax");
	__asm__ volatile("movl %0, %%cr3" : : "r"(cr3));
	__asm__ volatile("movl %%cr0, %%eax; orl $0x80010000, %%eax; movl %%eax, %%cr0" : : : "eax");
}

/*
 * Check CPUID for FXSR (EDX bit 24) and SSE (EDX bit 25).  If both are present,
 * set CR4.OSFXSR and CR4.OSXMMEXCPT, clear CR0.EM and set CR0.MP, and
 * initialize the current space's FXSAVE area with Linux's x87 control word
 * (0x037F: extended precision, every exception masked).  Sets sse_on.
 */
static void sse_init(struct gate_space *s)
{
	unsigned long eax = 1, ebx, ecx = 0, edx;
	unsigned long cr0, cr4;
	unsigned int i;

	/* CPUID leaf 1: feature flags in EDX; cpuid writes all four */
	__asm__ volatile("cpuid" : "+a"(eax), "=b"(ebx), "+c"(ecx), "=d"(edx));
	(void)ebx;

	/* bit 24: FXSR, bit 25: SSE */
	if (!((edx >> 24) & 1) || !((edx >> 25) & 1)) {
		/* no FXSR or SSE: leave CR0 and CR4 as they were, do not FXSAVE or FXRSTOR */
		sse_on = 0;
		return;
	}

	/* set CR4.OSFXSR (bit 9) and CR4.OSXMMEXCPT (bit 10) */
	__asm__ volatile("movl %%cr4, %0" : "=r"(cr4));
	cr4 |= (1 << 9) | (1 << 10);
	__asm__ volatile("movl %0, %%cr4" : : "r"(cr4));

	/* clear CR0.EM (bit 2), set CR0.MP (bit 1) */
	__asm__ volatile("movl %%cr0, %0" : "=r"(cr0));
	cr0 = (cr0 & ~(1UL << 2)) | (1UL << 1);
	__asm__ volatile("movl %0, %%cr0" : : "r"(cr0));

	sse_on = 1;

	/* initialize the first space's FXSAVE area: zero it, then set x87 CW to
	 * 0x037F (extended precision, every exception masked), as Linux gives a new program */
	for (i = 0; i < FXSAVE_SIZE; i++)
		s->fxsave[i] = 0;
	/* x87 control word is at offset 0 in the FXSAVE area (16-bit field) */
	*(unsigned short *)s->fxsave = 0x037f;
	/* MXCSR is at offset 24 in the FXSAVE area; Linux's default is 0x1f80 (all SSE exceptions masked) */
	*(unsigned int *)(s->fxsave + 24) = 0x1f80;

	/* load the initialized state into the processor */
	__asm__ volatile("fxrstor %0" : : "m"(s->fxsave));
}

void __attribute__((noreturn)) gate_enter(unsigned long eip, unsigned long esp, unsigned long eflags);
void __attribute__((noreturn)) gate_resume_frame(unsigned long cr3, unsigned long *frame);

/* the kernel and the stub page must fit under the first 4 MiB, and the window must start above it */
static int window_ok(unsigned long lo, unsigned long hi)
{
	return (unsigned long)kernel_end <= 0x400000 && lo >= 0x400000 && hi >= lo && hi <= 0xffc00000UL;
}

void __attribute__((noreturn)) gate_run(unsigned long entry, unsigned long lo, unsigned long hi,
	const struct door *kernel_calls)
{
	unsigned long *sp, top = (unsigned long)gate_stack_hi;
	struct gate_space *s = &spaces[0];

	if (!window_ok(lo, hi))
		kernel_halt();
	calls = kernel_calls;
	win_lo = lo;
	win_hi = hi;
	nspaces = 1;
	base_init(1);
	space_map(pds[0], lo, lo, hi - lo);
	s->cr3 = (unsigned long)pds[0];
	s->kstack_top = (unsigned long)stack_top;
	s->user_stack = 1;
	cur = s;
	tables_init();
	paging_on(s->cr3);
	sse_init(s);

	__asm__("" : "+r"(top));		/* the stack is a place the linker chose, not an object the compiler may reason about */
	sp = (unsigned long *)top;
	*--sp = (unsigned long)gate_door;	/* _start's argument */
	*--sp = (unsigned long)gate_return;	/* where _start returns to: exit */
	gate_enter(entry, (unsigned long)sp, 0x002);	/* interrupts off, I/O privilege 0 */
}

/*
 * A program from a Linux program file (PART.loader@10): its window [VIRT, VIRT +
 * LEN) is the physical RAM from PHYS, open to ring 3; it starts at ENTRY with its
 * stack at ESP (inside the window, laid out by the loader the way Linux lays it
 * out), and its heap starts at BRK.  No system call table is handed over: it
 * knocks on int 0x80 itself, as Linux programs do.
 */
void __attribute__((noreturn)) gate_exec(unsigned long entry, unsigned long phys, unsigned long virt,
	unsigned long len, unsigned long esp, unsigned long brk_start, const struct door *kernel_calls)
{
	struct gate_space *s = &spaces[0];

	if (!window_ok(virt, virt + len) || brk_start < virt || brk_start > virt + len)
		kernel_halt();
	calls = kernel_calls;
	win_lo = virt;
	win_hi = virt + len;
	brk_lo = brk_cur = brk_start;
	map_lo = esp & ~(PAGE - 1);
	if (map_lo > 0x100000UL + brk_lo)
		map_lo -= 0x100000UL;		/* the stack keeps 1 MiB below where it starts */
	nspaces = 1;
	base_init(0);
	space_map(pds[0], phys, virt, len);
	s->cr3 = (unsigned long)pds[0];
	s->kstack_top = (unsigned long)stack_top;
	s->user_stack = 0;
	cur = s;
	tables_init();
	paging_on(s->cr3);
	sse_init(s);
	gate_enter(entry, esp, 0x002);	/* interrupts off, I/O privilege 0 */
}

void gate_share_calls(const struct door *kernel_calls)
{
	calls = kernel_calls;
}

void gate_schedule(const struct gate_sched *s)
{
	sched = s;
}

struct gate_space *gate_space_new(unsigned long phys, unsigned long virt, unsigned long len,
	unsigned long entry, unsigned long stack_top_virt)
{
	struct gate_space *s;
	unsigned long *f, *us;
	unsigned int i;

	if (nspaces >= GATE_MAX_SPACES || !window_ok(virt, virt + len))
		return 0;
	if (nspaces && (virt != win_lo || virt + len != win_hi))
		return 0;			/* one window for every space */
	if (!nspaces) {
		win_lo = virt;
		win_hi = virt + len;
		base_init(0);
	}
	s = &spaces[nspaces];
	space_map(pds[nspaces], phys, virt, len);
	s->cr3 = (unsigned long)pds[nspaces];
	s->kstack_top = (unsigned long)&kstacks[nspaces][KSTACK_WORDS];
	s->user_stack = 0;

	/* initialize the new space's FXSAVE area if SSE is on */
	if (sse_on) {
		for (i = 0; i < FXSAVE_SIZE; i++)
			s->fxsave[i] = 0;
		/* x87 CW 0x037F: extended precision, every exception masked, as Linux gives */
		*(unsigned short *)s->fxsave = 0x037f;
		/* MXCSR at offset 24: 0x1f80, all SSE exceptions masked */
		*(unsigned int *)(s->fxsave + 24) = 0x1f80;
	}

	/* the program's stack, written through its physical address (paging is off): _start's argument and return */
	us = (unsigned long *)(phys + (stack_top_virt - virt));
	*--us = (unsigned long)gate_door;
	*--us = (unsigned long)gate_return;

	/* and the frame that resumes it, as if it had been trapped just before its first instruction */
	f = (unsigned long *)s->kstack_top;
	*--f = 0x23;				/* ss */
	*--f = stack_top_virt - 8;		/* esp */
	*--f = 0x202;				/* eflags: interrupts on, I/O privilege 0 */
	*--f = 0x1b;				/* cs */
	*--f = entry;				/* eip */
	*--f = 0;				/* error code */
	*--f = 0;				/* vector */
	*--f = 0x23;				/* ds */
	*--f = 0x23;				/* es */
	{
		int k;

		for (k = 0; k < 8; k++)
			*--f = 0;		/* edi esi ebp esp ebx edx ecx eax */
	}
	s->frame = f;
	nspaces++;
	return s;
}

void __attribute__((noreturn)) gate_resume(struct gate_space *s)
{
	cur = s;
	tss[1] = (unsigned int)s->kstack_top;
	gate_resume_frame(s->cr3, s->frame);
}

void __attribute__((noreturn)) gate_start(struct gate_space *first)
{
	cur = first;
	tables_init();
	paging_on(first->cr3);
	sse_init(first);
	gate_resume(first);
}
