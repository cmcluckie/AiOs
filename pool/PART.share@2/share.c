/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.share@2: the processor shared between programs, the way the Amiga did
 * it in 1985, now behind the gate.  @1's page tables, interrupt table and
 * register saving went to PART.gate@1, which runs every program in ring 3; what
 * is left here is turn-taking: which program is next, what to do when one
 * exits or is stopped, and the 8254's tick that takes the processor back.  Ours;
 * the 8259, the 8254 tick and the cpio reader are lifted parts this one
 * depends on.
 *
 * Each program gets its own 8 MiB of RAM, mapped by the gate to the same
 * addresses every program is linked at (0x400000, stack at the top, heap
 * between), so programs built for any earlier kernel run unchanged and cannot
 * see each other's memory, the kernel's or the screen's.  The 8254's counter 0
 * interrupts HZ (100) times a second through the 8259; on every tick the next
 * program that is still running gets the processor: preemptive, so a program
 * that never gives it back still gets only its turn.  A program the processor
 * stops is stopped alone, with an Amiga-style "Guru Meditation" line on the
 * console, and the others carry on.  When the last program exits, the machine
 * halts.
 *
 * Depends on PART.gate@1, PART.irq.pic@1, PART.timer.pit-tick@1,
 * PART.archive.cpio@1.  Exposes CAP.cpu.share@1.
 */

#include "share.h"
#include "gate.h"
#include "pic.h"
#include "pittick.h"
#include "earlycpio.h"

#define MAX_TASKS	GATE_MAX_SPACES
#define REGION_BASE	0x800000	/* physical: program i's RAM starts at 8 MiB + 8 MiB * i */
#define REGION_SIZE	0x800000	/* 8 MiB: two 4 MiB pages */
#define REGION_TOP	(PROGRAM_BASE + REGION_SIZE)	/* virtual: the end of every program's memory */
#define STACK_SIZE	0x4000		/* 16 KiB at the top of the region */

struct task {
	struct gate_space *space;
	struct heap heap;
	int alive;
};

static struct task tasks[MAX_TASKS];
static int ntasks;			/* 0: nothing is being shared */
static int cur;

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
		gate_resume(tasks[n].space);
	}
}

static void hex8(char *p, unsigned long v)
{
	int i;

	for (i = 7; i >= 0; i--, v >>= 4)
		p[i] = "0123456789ABCDEF"[v & 15];
}

/* A hardware interrupt arrived: the tick gives the processor to the next program. */
static struct gate_space *on_irq(unsigned long vec, struct gate_space *was)
{
	int n;

	if (vec == (unsigned long)PIC_VECTOR(PIT_IRQ)) {
		pic_eoi(PIT_IRQ);
		n = next_alive();
		if (n >= 0)
			cur = n;
		return tasks[cur].space;
	}
	return was;	/* spurious (the 8259's IRQ 7, the APIC's vector): nothing happened, and no EOI is owed */
}

/* The processor stopped the running program.  Stop that program alone.  The two
 * numbers are the vector and the address: the one it touched (page faults) or
 * the instruction it was on. */
static struct gate_space *on_fault(unsigned long vec, unsigned long addr, struct gate_space *was)
{
	static char msg[] = "\nGuru Meditation #XXXXXXXX.XXXXXXXX  program X stopped\n";
	int n;

	(void)was;
	hex8(msg + 18, vec);
	hex8(msg + 27, addr);
	msg[45] = (char)('1' + cur);
	kernel_say(msg, sizeof msg - 1);
	tasks[cur].alive = 0;
	n = next_alive();
	if (n < 0) {
		kernel_flush();
		kernel_halt();
	}
	cur = n;
	return tasks[cur].space;
}

static const struct gate_sched sched = { on_irq, on_fault };

/*
 * Set up program i from a cpio member: copy it into its region and make its
 * space.  Paging is still off: physical addresses.
 */
static int task_init(const struct hvm_start_info *si, int i, const void *data, unsigned long size)
{
	unsigned long base = REGION_BASE + (unsigned long)i * REGION_SIZE, k;
	const unsigned char *src = data;
	unsigned char *dst = (unsigned char *)base;

	if (size > REGION_SIZE - STACK_SIZE || !kernel_is_ram(si, base, base + REGION_SIZE))
		return -1;
	for (k = 0; k < size; k++)
		dst[k] = src[k];

	tasks[i].space = gate_space_new(base, PROGRAM_BASE, REGION_SIZE, PROGRAM_BASE, REGION_TOP);
	if (!tasks[i].space)
		return -1;
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

	gate_share_calls(door);
	while (ntasks < MAX_TASKS) {
		cd = find_cpio_data("", (char *)archive + at, len - (unsigned long)at, &next);
		if (!cd.data)
			break;
		if (task_init(si, ntasks, cd.data, cd.size))
			break;
		ntasks++;
		at += next;
	}
	if (!ntasks)
		kernel_halt();	/* an archive with nothing runnable in it */

	gate_schedule(&sched);
	pic_init();
	pit_tick_start();
	pic_unmask(PIT_IRQ);
	cur = 0;
	gate_start(tasks[0].space);
}
