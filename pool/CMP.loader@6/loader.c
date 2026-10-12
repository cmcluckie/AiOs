/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.loader@6: the KERNAL becomes a kernel.  Through @5 this was a KERNAL in
 * the Commodore sense: a table of services at fixed places (the door) and one
 * program.  @6 runs more than one program at once, and that is what makes a
 * kernel a kernel: something has to share the processor and keep the programs
 * out of each other's memory.  Ours; the 8259, the 8254 tick and the cpio
 * reader are lifted (lifted/).
 *
 * One program (the module is a plain program, as for every earlier rung): the
 * loader does exactly what @5 did.  No interrupts, no page tables, no timer;
 * the ladder's programs see no difference.
 *
 * More than one (the module is a cpio archive of programs, the format Linux's
 * initramfs uses): each program gets its own 8 MiB of RAM, mapped by its own
 * page directory to the same addresses every program is linked at (0x400000,
 * stack at the top, heap between), so programs built for any earlier loader
 * run unchanged and cannot see each other's memory.  The 8254's counter 0
 * interrupts HZ (100) times a second through the 8259; on every tick the
 * kernel saves the running program and resumes the next one: preemptive, the
 * way the Amiga did it in 1985, so a program that never gives the processor
 * back still gets only its turn.  A program that faults (touches memory that
 * is not its own, divides by zero, anything the processor refuses) is stopped
 * alone, with an Amiga-style "Guru Meditation" line on the console, and the
 * others carry on.  When the last program exits, the machine halts.
 *
 * Not yet: programs run with the same privilege as the kernel (no ring 3), so
 * one could still scribble on the kernel's own memory (identity-mapped and
 * writable from 1 MiB to kernel_end in every address space) or turn paging
 * off.  The door is a table of function pointers, and a program with less
 * power could not call it; that is the next door (findings 0011).
 *
 * @2 alloc, @3 read, @4 ms, @5 load and save are as before; alloc now hands
 * out memory from the calling program's own region.  The structs below are
 * the x86/HVM direct boot ABI ("PVH"), a public interface: Xen's
 * public/arch-x86/hvm/start_info.h, MIT, Copyright (c) 2016 Citrix Systems,
 * Inc.  Layout reproduced; that is what an ABI is for.  The page table and
 * descriptor bits are the Intel SDM's.
 *
 * Depends on CMP.serial.8250@2, CMP.timer.tsc-pit@1, CMP.fs.minix@1,
 * CMP.disk.ata-pio@1, CMP.irq.pic@1, CMP.timer.pit-tick@1 and
 * CMP.archive.cpio@1 (lifted/).
 * Exposes CAP.proc.exit, CAP.mem.alloc, CAP.time.read, CAP.fs.read,
 * CAP.fs.write and CAP.cpu.share.
 */

#include "door.h"
#include "../../lifted/CMP.serial.8250@2/serial8250.h"
#include "../../lifted/CMP.timer.tsc-pit@1/tsccal.h"
#include "../../lifted/CMP.fs.minix@1/minix.h"
#include "../../lifted/CMP.irq.pic@1/pic.h"
#include "../../lifted/CMP.timer.pit-tick@1/pittick.h"
#include "../../lifted/CMP.archive.cpio@1/earlycpio.h"

#define CONSOLE_PORT	0x3f8		/* CAP.dev.serial */
#define PROGRAM_BASE	0x400000	/* where every program runs; its link address */

/* PVH start info, version 1 */
#define XEN_HVM_START_MAGIC_VALUE 0x336ec578

struct hvm_start_info {
	unsigned int   magic;
	unsigned int   version;
	unsigned int   flags;
	unsigned int   nr_modules;
	unsigned long long modlist_paddr;
	unsigned long long cmdline_paddr;
	unsigned long long rsdp_paddr;
	unsigned long long memmap_paddr;
	unsigned int   memmap_entries;
	unsigned int   reserved;
};

struct hvm_modlist_entry {
	unsigned long long paddr;
	unsigned long long size;
	unsigned long long cmdline_paddr;
	unsigned long long reserved;
};

struct hvm_memmap_table_entry {
	unsigned long long addr;
	unsigned long long size;
	unsigned int   type;		/* 1 = RAM; everything else is not ours */
	unsigned int   reserved;
};
#define MEMMAP_RAM 1

/*
 * The programs.  With one plain program, task 0 is all there is and nothing
 * below the heap fields is used.
 */
#define MAX_TASKS	4
#define REGION_BASE	0x800000	/* physical: program i's RAM starts at 8 MiB + 8 MiB * i */
#define REGION_SIZE	0x800000	/* 8 MiB: two 4 MiB pages */
#define REGION_TOP	(PROGRAM_BASE + REGION_SIZE)	/* virtual: the end of every program's memory */
#define STACK_SIZE	0x4000		/* 16 KiB at the top of the region */

struct task {
	unsigned long esp;		/* where its registers were saved, in its own address space */
	unsigned long cr3;		/* its page directory */
	unsigned long heap_next, heap_end;
	int alive;
};

static struct task tasks[MAX_TASKS];
static int ntasks;			/* 0: one plain program, the @5 way */
static int cur;
unsigned long next_cr3;			/* read by isr.S on the way out of every trap */

static unsigned long page_dir[MAX_TASKS][1024] __attribute__((aligned(4096)));
static unsigned long low_pt[1024] __attribute__((aligned(4096)));
extern char kernel_end[];

static void __attribute__((noreturn)) halt(void);

/* Interrupts off for the length of a door call, so a tick never lands in the
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

	tasks[0].heap_next = (program_end + 15) & ~15UL;
	tasks[0].heap_end = tasks[0].heap_next;	/* no map, no memory: alloc refuses everything */
	if (si->version < 1 || !si->memmap_entries)
		return;
	for (i = 0; i < si->memmap_entries; i++) {
		unsigned long lo = (unsigned long)m[i].addr;
		unsigned long hi = (unsigned long)(m[i].addr + m[i].size);

		if (m[i].type == MEMMAP_RAM && lo <= PROGRAM_BASE && PROGRAM_BASE < hi) {
			tasks[0].heap_end = hi;
			break;
		}
	}
}

/* Is [lo, hi) all RAM, by the firmware's map? */
static int is_ram(const struct hvm_start_info *si, unsigned long lo, unsigned long hi)
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
	struct task *t = &tasks[cur];
	unsigned long p = t->heap_next, n = (bytes + 15) & ~15UL;
	void *r = 0;

	if (!(bytes == 0 || n < bytes || n > t->heap_end - p)) {
		t->heap_next = p + n;
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
static void __attribute__((noreturn)) halt(void)
{
	static const struct { unsigned short limit; unsigned int base; } __attribute__((packed)) null_idt = { 0, 0 };

	__asm__ volatile("cli; lidt %0; int3" : : "m"(null_idt));
	for (;;)
		__asm__ volatile("cli; hlt");
}

static void door_write(const void *s, unsigned long n)
{
	unsigned long f = irq_save();

	serial8250_write(CONSOLE_PORT, s, n);
	irq_restore(f);
}

/* Wait for one byte, then take whatever else has already arrived.  While a
 * program waits, the others keep their turns. */
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
 * measured once at boot; start is the counter then.
 */
static unsigned long clock_khz;
static unsigned long long clock_start;

static unsigned long door_ms(void)
{
	unsigned long long t;
	unsigned long hi, lo, q, r;

	if (!clock_khz)
		return 0;
	t = tsccal_cycles() - clock_start;
	hi = (unsigned long)(t >> 32);
	lo = (unsigned long)t;
	if (hi >= clock_khz)
		return 0xffffffffUL;	/* 49 days and 17 hours: the door's last value, held */
	__asm__("divl %4" : "=a"(q), "=d"(r) : "0"(lo), "1"(hi), "rm"(clock_khz));
	(void)r;
	return q;
}

static long door_load(const char *name, void *buf, unsigned long max)
{
	unsigned long f = irq_save();
	long r = minix_load(name, buf, max);

	irq_restore(f);
	return r;
}

static long door_save(const char *name, const void *buf, unsigned long n)
{
	unsigned long f = irq_save();
	long r = minix_save(name, buf, n);

	irq_restore(f);
	return r;
}

/* ---- sharing: only when the module is an archive of programs ---- */

void __attribute__((noreturn)) task_resume(unsigned long cr3, unsigned long esp);

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

static void __attribute__((noreturn)) door_exit(int code);

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
		serial8250_write(CONSOLE_PORT, msg, sizeof msg - 1);
	}
	tasks[cur].alive = 0;
	n = next_alive();
	if (n < 0) {
		serial8250_flush(CONSOLE_PORT);
		halt();
	}
	cur = n;
	next_cr3 = tasks[cur].cr3;
	return tasks[cur].esp;
}

static void __attribute__((noreturn)) door_exit(int code)
{
	int n;

	(void)code;			/* unobservable from outside the machine; findings 0000 #9 */
	__asm__ volatile("cli");
	if (ntasks) {
		tasks[cur].alive = 0;
		n = next_alive();
		if (n >= 0) {
			cur = n;
			task_resume(tasks[n].cr3, tasks[n].esp);
		}
	}
	serial8250_flush(CONSOLE_PORT);
	halt();
}

/* A program that returns from _start has exited. */
static void task_return(void)
{
	door_exit(0);
}

static const struct door door = { door_write, door_exit, door_alloc, door_read, door_ms, door_load, door_save };

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
 * _start(&door) with interrupts on.  Paging is still off: physical addresses.
 */
static int task_init(const struct hvm_start_info *si, int i, const void *data, unsigned long size)
{
	unsigned long base = REGION_BASE + (unsigned long)i * REGION_SIZE;
	unsigned long *pd = page_dir[i], *sp, k;
	const unsigned char *src = data;
	unsigned char *dst = (unsigned char *)base;

	if (size > REGION_SIZE - STACK_SIZE || !is_ram(si, base, base + REGION_SIZE))
		return -1;
	for (k = 0; k < size; k++)
		dst[k] = src[k];

	pd[0] = (unsigned long)low_pt | 0x3;			/* the kernel, 4 KiB pages: present, writable */
	pd[PROGRAM_BASE >> 22] = base | 0x83;			/* 4 MiB page: present, writable, large */
	pd[(PROGRAM_BASE >> 22) + 1] = (base + 0x400000) | 0x83;

	sp = (unsigned long *)(base + REGION_SIZE);		/* the top of the stack, physically */
	*--sp = (unsigned long)&door;				/* _start's argument */
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
	tasks[i].heap_next = (PROGRAM_BASE + size + 15) & ~15UL;
	tasks[i].heap_end = REGION_TOP - STACK_SIZE;
	tasks[i].alive = 1;
	return 0;
}

static void __attribute__((noreturn)) share(const struct hvm_start_info *si, void *archive, unsigned long len)
{
	struct cpio_data cd;
	long next = 0, at = 0;
	unsigned long k;

	/* the kernel's own pages, the same in every address space: 1 MiB to its end */
	for (k = 0x100000 >> 12; k < (((unsigned long)kernel_end + 4095) >> 12); k++)
		low_pt[k] = (k << 12) | 0x3;

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
		halt();		/* an archive with nothing runnable in it */

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

void __attribute__((noreturn)) loader_main(const struct hvm_start_info *si)
{
	const struct hvm_modlist_entry *mod;
	const unsigned char *src;
	int tries;
	unsigned char *dst;
	unsigned long n;

	serial8250_init(CONSOLE_PORT);
	/* About 50 ms against the 8254 per try, then never again.  Findings 0008. */
	for (tries = 0; tries < 5 && !clock_khz; tries++)
		clock_khz = tsccal_khz();
	clock_start = tsccal_cycles();

	if (si->magic != XEN_HVM_START_MAGIC_VALUE || si->nr_modules < 1)
		halt();		/* no program: nothing to say, nothing to do */

	mod = (const struct hvm_modlist_entry *)(unsigned long)si->modlist_paddr;
	src = (const unsigned char *)(unsigned long)mod[0].paddr;

	/* An archive of programs ("070701", cpio's new ASCII header): share. */
	if (mod[0].size > 6 && src[0] == '0' && src[1] == '7' && src[2] == '0' &&
	    src[3] == '7' && src[4] == '0' && src[5] == '1')
		share(si, (void *)src, (unsigned long)mod[0].size);

	/* One plain program, exactly as @5: copy it to PROGRAM_BASE and call it. */
	dst = (unsigned char *)PROGRAM_BASE;
	for (n = (unsigned long)mod[0].size; n; n--)
		*dst++ = *src++;
	heap_init(si, (unsigned long)dst);

	((program_entry)PROGRAM_BASE)(&door);
	door_exit(0);		/* a program that returns has exited */
}
