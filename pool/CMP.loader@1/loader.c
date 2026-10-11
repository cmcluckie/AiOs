/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.loader@1: takes over from the machine, runs one program, halts when it exits.
 * The whole landlord, for a building with one tenant.
 *
 * Lifted from Linux, commit a5ebb76233b7:
 *   include/xen/interface/hvm/start_info.h   struct hvm_start_info and
 *                                            struct hvm_modlist_entry, 111-134,
 *                                            XEN_HVM_START_MAGIC_VALUE 86
 * Everything else here is ours.  No scheduler, no memory manager, no interrupts,
 * no page tables: there is one program and it owns everything the door lets it reach.
 *
 * Depends on CMP.serial.8250@1 for the write door.
 */

#include "door.h"
#include "../CMP.serial.8250@1/serial8250.h"

#define CONSOLE_PORT	0x3f8		/* CAP.dev.serial: the machine's one peripheral */
#define PROGRAM_BASE	0x400000	/* where the program runs; its link address */

/* include/xen/interface/hvm/start_info.h */
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

static const struct door door = { door_write, door_exit };

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

	((program_entry)PROGRAM_BASE)(&door);
	door_exit(0);		/* a program that returns has exited */
}
