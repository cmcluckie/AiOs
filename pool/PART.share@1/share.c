/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.share@1: the processor shared between programs, the way the Amiga did
 * it in 1985.  Moved out of PART.loader@6 and @7, where it was always built
 * in, into a part of its own, so a kernel has it only when a petition asks to
 * share (plan 5.8, "the petition is the partition").  The code is @7's, moved,
 * not rewritten; what changed is only how it reaches the rest of the kernel
 * (kernel.h) instead of calling the serial part itself.  Ours; the 8259, the
 * 8254 tick and the cpio reader are lifted parts this one depends on.
 *
 * Each program gets its own 8 MiB of RAM, mapped by its own page directory to
 * the same addresses every program is linked at (0x400000, stack at the top,
 * heap between), so programs built for any earlier kernel run unchanged and
 * cannot see each other's memory.  The 8254's counter 0 interrupts HZ (100)
 * times a second through the 8259; on every tick the kernel saves the running
 * program and resumes the next one: preemptive, so a program that never gives
 * the processor back still gets only its turn.  A program that faults is
 * stopped alone, with an Amiga-style "Guru Meditation" line on the console,
 * and the others carry on.  When the last program exits, the machine halts.
 *
 * Not yet: programs run with the same privilege as the kernel (no ring 3)
 * (findings 0011).  The page table and descriptor bits are the Intel SDM's.
 *
 * Depends on PART.irq.pic@1, PART.timer.pit-tick@1, PART.archive.cpio@1.
 * Exposes CAP.cpu.share@1.
 */

#include "share.h"
#include "pic.h"
#include "pittick.h"
#include "earlycpio.h"

#define MAX_TASKS	4
#define REGION_BASE	0x800000	/* physical: program i's RAM starts at 8 MiB + 8 MiB * i */
#define REGION_SIZE	0x800000	/* 8 MiB: two 4 MiB pages */
#define REGION_TOP	(PROGRAM_BASE + REGION_SIZE)	/* virtual: the end of every program's memory */
#define STACK_SIZE	0x4000		/* 16 KiB at the top of the region */

struct task {
	unsigned long esp;		/* where its registers were saved, in its own address space */
	unsigned long cr3;		/* its page directory */
	struct heap heap;
	int alive;
};

static struct task tasks[MAX_TASKS];
static int ntasks;			/* 0: nothing is being shared */
static int cur;
unsigned long next_cr3;			/* read by isr.S on the way out of every trap */

static unsigned long page_dir[MAX_TASKS][1024] __attribute__((aligned(4096)));
static unsigned long low_pt[1024] __attribute__((aligned(4096)));
extern char kernel_end[];

void __attribute__((noreturn)) task_resume(unsigned long cr3, unsigned long esp);

static const struct door *the_door;

/* A program that returns from _start has exited. */
static void task_return(void)
{
	the_door->exit(0);
}

struct heap *share_heap(void)
{
	return ntasks ? &tasks[cur].heap : 0;
}

/* The next program that is still running, after the current one; -1 if none. */
static int next_alive(void)
{
	int i, n;

	for (i = 1; i <= ntasks; i++) {
		n = (cur + i) % ntasks;
		if (tasks[n].alive)
			return n;
	}
	return -1;
}

void share_exit(void)
{
	int n;

	if (!ntasks)
		return;
	tasks[cur].alive = 0;
	n = next_alive();
	if (n >= 0) {
		cur = n;
		task_resume(tasks[n].cr3, tasks[n].esp);
	}
}

static void hex8(char *p, unsigned long v)
{
	int i;

	for (i = 7; i >= 0; i--, v >>= 4)
		p[i] = "0123456789ABCDEF"[v & 15];
}

/*
 * Every trap.  frame points at what isr.S saved: eight registers (popal
 * order), the vector, the error code, eip, cs, eflags.  Returns the stack to
 * resume, and leaves its page directory in next_cr3.
 */
unsigned long kernel_trap(unsigned long *frame)
{
	unsigned long vec = frame[8];
	int n;

	next_cr3 = tasks[cur].cr3;
	if (vec == (unsigned long)PIC_VECTOR(PIT_IRQ)) {
		pic_eoi(PIT_IRQ);
		tasks[cur].esp = (unsigned long)frame;
		n = next_alive();
		if (n >= 0)
			cur = n;
		next_cr3 = tasks[cur].cr3;
		return tasks[cur].esp;
	}
	if (vec == (unsigned long)PIC_VECTOR(7) || vec == PIC_SPURIOUS_APIC_VECTOR)
		return (unsigned long)frame;	/* spurious: nothing happened, and no EOI is owed */

	/* The processor refused something the program did.  Stop that program
	 * alone.  The two numbers are the vector and the address: the one it
	 * touched (page faults) or the instruction it was on. */
	{
		static char msg[] = "\nGuru Meditation #XXXXXXXX.XXXXXXXX  program X stopped\n";
		unsigned long addr;

		if (vec == 14)
			__asm__ volatile("movl %%cr2, %0" : "=r"(addr));
		else
			addr = frame[10];
		hex8(msg + 18, vec);
		hex8(msg + 27, addr);
		msg[45] = (char)('1' + cur);
		kernel_say(msg, sizeof msg - 1);
	}
	tasks[cur].alive = 0;
	n = next_alive();
	if (n < 0) {
		kernel_flush();
		kernel_halt();
	}
	cur = n;
	next_cr3 = tasks[cur].cr3;
	return tasks[cur].esp;
}

/* The IDT: a gate for each vector isr.S has an entry for, none for the rest
 * (an interrupt on any other vector is a triple fault: the machine stops). */
static unsigned long long idt[256];

static void set_gate(int v, const char *handler)
{
	unsigned long h = (unsigned long)handler;

	/* present, ring 0, 32-bit interrupt gate (interrupts off on entry), code selector 0x08 */
	idt[v] = (h & 0xffff) | (0x08UL << 16) | ((unsigned long long)(0x8e00 | (h & 0xffff0000)) << 32);
}

static void idt_init(void)
{
	static struct { unsigned short limit; unsigned int base; } __attribute__((packed)) d;
	extern char trap0[], trap1[], trap2[], trap3[], trap4[], trap5[], trap6[], trap7[],
		trap8[], trap9[], trap10[], trap11[], trap12[], trap13[], trap14[], trap15[],
		trap16[], trap17[], trap18[], trap19[], trap20[], trap21[], trap22[], trap23[],
		trap24[], trap25[], trap26[], trap27[], trap28[], trap29[], trap30[], trap31[],
		trap0x30[], trap0x37[], trap0xff[];
	char *t[32] = { trap0, trap1, trap2, trap3, trap4, trap5, trap6, trap7,
		trap8, trap9, trap10, trap11, trap12, trap13, trap14, trap15,
		trap16, trap17, trap18, trap19, trap20, trap21, trap22, trap23,
		trap24, trap25, trap26, trap27, trap28, trap29, trap30, trap31 };
	int i;

	for (i = 0; i < 32; i++)
		set_gate(i, t[i]);
	set_gate(PIC_VECTOR(PIT_IRQ), trap0x30);
	set_gate(PIC_VECTOR(7), trap0x37);
	set_gate(PIC_SPURIOUS_APIC_VECTOR, trap0xff);
	d.limit = sizeof idt - 1;
	d.base = (unsigned int)idt;
	__asm__ volatile("lidt %0" : : "m"(d));
}

/*
 * Set up program i from a cpio member: copy it into its region, build its page
 * directory, and leave a frame at the top of its stack that resumes at
 * _start(door) with interrupts on.  Paging is still off: physical addresses.
 */
static int task_init(const struct hvm_start_info *si, int i, const void *data, unsigned long size,
	const struct door *door)
{
	unsigned long base = REGION_BASE + (unsigned long)i * REGION_SIZE;
	unsigned long *pd = page_dir[i], *sp, k;
	const unsigned char *src = data;
	unsigned char *dst = (unsigned char *)base;

	if (size > REGION_SIZE - STACK_SIZE || !kernel_is_ram(si, base, base + REGION_SIZE))
		return -1;
	for (k = 0; k < size; k++)
		dst[k] = src[k];

	pd[0] = (unsigned long)low_pt | 0x3;			/* the kernel, 4 KiB pages: present, writable */
	pd[PROGRAM_BASE >> 22] = base | 0x83;			/* 4 MiB page: present, writable, large */
	pd[(PROGRAM_BASE >> 22) + 1] = (base + 0x400000) | 0x83;

	sp = (unsigned long *)(base + REGION_SIZE);		/* the top of the stack, physically */
	*--sp = (unsigned long)door;				/* _start's argument */
	*--sp = (unsigned long)task_return;			/* where _start returns to */
	*--sp = 0x202;						/* eflags: interrupts on */
	*--sp = 0x08;						/* cs */
	*--sp = PROGRAM_BASE;					/* eip */
	*--sp = 0;						/* error code */
	*--sp = 0;						/* vector */
	for (k = 0; k < 8; k++)
		*--sp = 0;					/* eax ecx edx ebx esp ebp esi edi, popal's order reversed */

	tasks[i].esp = REGION_TOP - (base + REGION_SIZE - (unsigned long)sp);
	tasks[i].cr3 = (unsigned long)pd;
	tasks[i].heap.next = (PROGRAM_BASE + size + 15) & ~15UL;
	tasks[i].heap.end = REGION_TOP - STACK_SIZE;
	tasks[i].alive = 1;
	return 0;
}

void __attribute__((noreturn)) share(const struct hvm_start_info *si, void *archive, unsigned long len,
	const struct door *door)
{
	struct cpio_data cd;
	long next = 0, at = 0;
	unsigned long k;

	the_door = door;
	/* the kernel's own pages, the same in every address space: 1 MiB to its end */
	for (k = 0x100000 >> 12; k < (((unsigned long)kernel_end + 4095) >> 12); k++)
		low_pt[k] = (k << 12) | 0x3;

	while (ntasks < MAX_TASKS) {
		cd = find_cpio_data("", (char *)archive + at, len - (unsigned long)at, &next);
		if (!cd.data)
			break;
		if (task_init(si, ntasks, cd.data, cd.size, door))
			break;
		ntasks++;
		at += next;
	}
	if (!ntasks)
		kernel_halt();	/* an archive with nothing runnable in it */

	idt_init();
	pic_init();
	pit_tick_start();
	pic_unmask(PIT_IRQ);

	/* paging on: 4 MiB pages allowed, then the first program's directory */
	__asm__ volatile("movl %%cr4, %%eax; orl $0x10, %%eax; movl %%eax, %%cr4" : : : "eax");
	__asm__ volatile("movl %0, %%cr3" : : "r"(tasks[0].cr3));
	__asm__ volatile("movl %%cr0, %%eax; orl $0x80000000, %%eax; movl %%eax, %%cr0" : : : "eax");
	cur = 0;
	task_resume(tasks[0].cr3, tasks[0].esp);
}
