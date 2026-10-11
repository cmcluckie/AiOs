/* SPDX-License-Identifier: MIT
 *
 * CMP.loader@2: takes over from the machine, runs one program, hands it memory
 * when asked, halts when it exits.  The whole landlord, for a building with one
 * tenant.  Ours; nothing lifted.  @2 adds the alloc door: the first resource the
 * landlord manages, found by reading the memory map the firmware hands over.
 *
 * The structs below are the x86/HVM direct boot ABI ("PVH"), a public
 * interface: Xen's public/arch-x86/hvm/start_info.h, MIT, Copyright (c) 2016
 * Citrix Systems, Inc.  Layout reproduced; that is what an ABI is for.
 *
 * No scheduler, no interrupts, no page tables.  The memory manager is a bump
 * pointer over one RAM region: there is one program, nothing is ever freed, and
 * nothing is protected because there is nobody to protect it from.
 *
 * Depends on CMP.serial.8250@1 (lifted/) for the write door.  Exposes
 * CAP.proc.exit and CAP.mem.alloc.
 */

#include "door.h"
#include "../../lifted/CMP.serial.8250@1/serial8250.h"

#define CONSOLE_PORT	0x3f8		/* CAP.dev.serial: the machine's one peripheral */
#define PROGRAM_BASE	0x400000	/* where the program runs; its link address */

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
 * The heap: from the end of the program to the end of the RAM region the
 * program lives in.  The loader (1 MiB) and its stack sit below the program;
 * the firmware's tables sit below 1 MiB; the module's original copy, above us,
 * is free once copied.  Nothing else in the map is ours.
 */
static unsigned long heap_next, heap_end;

static void heap_init(const struct hvm_start_info *si, unsigned long program_end)
{
	const struct hvm_memmap_table_entry *m = (const void *)(unsigned long)si->memmap_paddr;
	unsigned int i;

	heap_next = (program_end + 15) & ~15UL;
	heap_end = heap_next;		/* no map, no memory: alloc refuses everything */
	if (si->version < 1 || !si->memmap_entries)
		return;
	for (i = 0; i < si->memmap_entries; i++) {
		unsigned long lo = (unsigned long)m[i].addr;
		unsigned long hi = (unsigned long)(m[i].addr + m[i].size);

		if (m[i].type == MEMMAP_RAM && lo <= PROGRAM_BASE && PROGRAM_BASE < hi) {
			heap_end = hi;
			break;
		}
	}
}

static void *door_alloc(unsigned long bytes)
{
	unsigned long p = heap_next, n = (bytes + 15) & ~15UL;

	if (bytes == 0 || n < bytes || n > heap_end - p)
		return 0;		/* refused; the program decides what that means */
	heap_next = p + n;
	return (void *)p;
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

	__asm__ volatile("lidt %0; int3" : : "m"(null_idt));
	for (;;)
		__asm__ volatile("cli; hlt");
}

static void door_write(const void *s, unsigned long n)
{
	serial8250_write(CONSOLE_PORT, s, n);
}

static void __attribute__((noreturn)) door_exit(int code)
{
	(void)code;			/* unobservable from outside the machine; findings 0000 #9 */
	serial8250_flush(CONSOLE_PORT);
	halt();
}

static const struct door door = { door_write, door_exit, door_alloc };

void __attribute__((noreturn)) loader_main(const struct hvm_start_info *si)
{
	const struct hvm_modlist_entry *mod;
	const unsigned char *src;
	unsigned char *dst;
	unsigned long n;

	serial8250_init(CONSOLE_PORT);

	if (si->magic != XEN_HVM_START_MAGIC_VALUE || si->nr_modules < 1)
		halt();		/* no program: nothing to say, nothing to do */

	/* The firmware put the program (-initrd) wherever it liked; the program is
	 * linked at PROGRAM_BASE, so move it there.  Module 0 is the program. */
	mod = (const struct hvm_modlist_entry *)(unsigned long)si->modlist_paddr;
	src = (const unsigned char *)(unsigned long)mod[0].paddr;
	dst = (unsigned char *)PROGRAM_BASE;
	for (n = (unsigned long)mod[0].size; n; n--)
		*dst++ = *src++;
	heap_init(si, (unsigned long)dst);

	((program_entry)PROGRAM_BASE)(&door);
	door_exit(0);		/* a program that returns has exited */
}
