/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.gate@1: the gate.  Programs run in ring 3, the processor's least
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
 * TECH-DEBT 1: alloc, ms, load and save have no Linux number yet, so they use
 * AiOs-only numbers from 1000 (findings 0021 #5).  They go when Linux's program
 * files can be loaded (part 20): alloc becomes brk, ms becomes clock_gettime,
 * load and save become open, read, write and close.
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
#define NR_ALLOC	1000		/* TECH-DEBT 1 */
#define NR_MS		1001		/* TECH-DEBT 1 */
#define NR_LOAD		1002		/* TECH-DEBT 1 */
#define NR_SAVE		1003		/* TECH-DEBT 1 */

#define EFAULT		14
#define EBADF		9
#define ENOSYS		38
#define NAME_MAX_LEN	256		/* the longest file name the kernel will read from a program */

extern char kernel_end[], stack_top[];
extern char gate_user_start[], gate_stack_lo[], gate_stack_hi[];
extern char gate_door[], gate_return[];

static unsigned long long gdt[6];
static unsigned int tss[26];
static unsigned long long idt[256];
static unsigned long pd[1024] __attribute__((aligned(4096)));
static unsigned long pt0[1024] __attribute__((aligned(4096)));

static unsigned long win_lo, win_hi;
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
	return p >= (unsigned long)gate_stack_lo && e <= (unsigned long)gate_stack_hi;
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

/* frame: what trap_common saved (gate_entry.S).  Returns the call's answer. */
static long syscall(const unsigned long *frame)
{
	unsigned long nr = frame[7], a = frame[4], b = frame[6], c = frame[5];	/* eax; ebx, ecx, edx */

	switch (nr) {
	case NR_EXIT:
		calls->exit((int)a);
	case NR_WRITE:
		if (a != 1 && a != 2)
			return -EBADF;
		if (!user_ok(b, c))
			return -EFAULT;
		calls->write((const void *)b, c);
		return (long)c;
	case NR_READ:
		if (a != 0)
			return -EBADF;
		if (!user_ok(b, c))
			return -EFAULT;
		return (long)calls->read((void *)b, c);
	case NR_ALLOC:
		return (long)calls->alloc(a);
	case NR_MS:
		return (long)calls->ms();
	case NR_LOAD:
		if (!user_name_ok((const char *)a) || !user_ok(b, c))
			return -EFAULT;
		return calls->load((const char *)a, (void *)b, c);
	case NR_SAVE:
		if (!user_name_ok((const char *)a) || !user_ok(b, c))
			return -EFAULT;
		return calls->save((const char *)a, (const void *)b, c);
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
	static char msg[] = "\nGuru Meditation #XXXXXXXX.XXXXXXXX  program 1 stopped\n";

	if (vec == 0x80) {
		frame[7] = (unsigned long)syscall(frame);
		return frame;
	}
	/* The processor refused something.  The two numbers are the vector and the
	 * address: the one it touched (page faults) or the instruction it was on. */
	if (vec == 14)
		__asm__ volatile("movl %%cr2, %0" : "=r"(addr));
	else
		addr = frame[12];
	hex8(msg + 18, vec);
	hex8(msg + 27, addr);
	kernel_say(msg, sizeof msg - 1);
	kernel_flush();
	kernel_halt();
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
	id.limit = sizeof idt - 1;
	id.base = (unsigned int)idt;
	__asm__ volatile("lidt %0" : : "m"(id));
}

/* Every address maps to itself.  Ring 3 sees only its window, its stack and the stub page. */
static void paging_on(void)
{
	unsigned long i, stub = (unsigned long)gate_user_start >> 12;

	for (i = 0; i < 1024; i++)
		pt0[i] = (i << 12) | 0x3;	/* present, writable, kernel only */
	pt0[stub] = (stub << 12) | 0x5;		/* present, read-only, ring 3 may read and run it */
	for (i = (unsigned long)gate_stack_lo >> 12; i < (unsigned long)gate_stack_hi >> 12; i++)
		pt0[i] |= 0x4;			/* ring 3's stack */
	pd[0] = (unsigned long)pt0 | 0x7;	/* the first 4 MiB: page by page */
	for (i = 1; i < 1024; i++)
		pd[i] = (i << 22) | 0x83;	/* 4 MiB pages, present, writable, kernel only */
	for (i = win_lo >> 22; i < (win_hi + 0x3fffffUL) >> 22; i++)
		pd[i] |= 0x4;			/* the program's window */

	__asm__ volatile("movl %%cr4, %%eax; orl $0x10, %%eax; movl %%eax, %%cr4" : : : "eax");
	__asm__ volatile("movl %0, %%cr3" : : "r"(pd));
	__asm__ volatile("movl %%cr0, %%eax; orl $0x80010000, %%eax; movl %%eax, %%cr0" : : : "eax");
}

void __attribute__((noreturn)) gate_enter(unsigned long eip, unsigned long esp, unsigned long eflags);

void __attribute__((noreturn)) gate_run(unsigned long entry, unsigned long lo, unsigned long hi,
	const struct door *kernel_calls)
{
	unsigned long *sp, top = (unsigned long)gate_stack_hi;

	/* the kernel and the stub page must fit under the first 4 MiB, and the window must start above it */
	if ((unsigned long)kernel_end > 0x400000 || lo < 0x400000 || hi < lo || hi > 0xffc00000UL)
		kernel_halt();
	calls = kernel_calls;
	win_lo = lo;
	win_hi = hi;
	tables_init();
	paging_on();

	__asm__("" : "+r"(top));		/* the stack is a place the linker chose, not an object the compiler may reason about */
	sp = (unsigned long *)top;
	*--sp = (unsigned long)gate_door;	/* _start's argument */
	*--sp = (unsigned long)gate_return;	/* where _start returns to: exit */
	gate_enter(entry, (unsigned long)sp, 0x002);	/* interrupts off, I/O privilege 0 */
}
