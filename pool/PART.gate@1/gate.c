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
 * Several programs (PART.share@2): each gets a space, its own page directory
 * that maps the same virtual window to its own physical RAM, and its own kernel
 * stack, where the processor puts a trap's frame.  The gate saves and resumes
 * the frames; who runs next is decided by the scheduler the sharing part
 * registers (gate_schedule), not here.
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

#define KSTACK_WORDS	4096		/* 16 KiB of kernel stack for each space */

struct gate_space {
	unsigned long cr3;		/* its page directory */
	unsigned long *frame;		/* where its registers were saved, on its kernel stack */
	unsigned long kstack_top;	/* where a trap from ring 3 lands (the TSS's esp0) */
	int user_stack;			/* ring 3 stands on the gate's own stack (one program, no sharing) */
};

static unsigned long long gdt[6];
static unsigned int tss[26];
static unsigned long long idt[256];
static unsigned long pds[GATE_MAX_SPACES][1024] __attribute__((aligned(4096)));
static unsigned long pt0[1024] __attribute__((aligned(4096)));
static unsigned long kstacks[GATE_MAX_SPACES][KSTACK_WORDS] __attribute__((aligned(16)));
static struct gate_space spaces[GATE_MAX_SPACES];
static int nspaces;
static struct gate_space *cur;
static const struct gate_sched *sched;

static unsigned long win_lo, win_hi;	/* the same virtual window in every space */
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
	struct gate_space *next = cur;
	static char msg[] = "\nGuru Meditation #XXXXXXXX.XXXXXXXX  program 1 stopped\n";

	cur->frame = frame;
	if (vec == 0x80) {
		frame[7] = (unsigned long)syscall(frame);
		return frame;
	}
	if (sched && ((vec & ~0xfUL) == 0x30 || vec == 0xff)) {
		/* a hardware interrupt: the scheduler may take the processor away */
		next = sched->irq(vec, cur);
	} else {
		/* The processor refused something.  The two numbers are the vector and the
		 * address: the one it touched (page faults) or the instruction it was on. */
		if (vec == 14)
			__asm__ volatile("movl %%cr2, %0" : "=r"(addr));
		else
			addr = frame[12];
		if (sched && (frame[13] & 3) == 3) {
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

	__asm__("" : "+r"(top));		/* the stack is a place the linker chose, not an object the compiler may reason about */
	sp = (unsigned long *)top;
	*--sp = (unsigned long)gate_door;	/* _start's argument */
	*--sp = (unsigned long)gate_return;	/* where _start returns to: exit */
	gate_enter(entry, (unsigned long)sp, 0x002);	/* interrupts off, I/O privilege 0 */
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
	gate_resume(first);
}
