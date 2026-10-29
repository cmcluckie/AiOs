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
