/* PART.loader@8: what the kernel offers the parts built into it.  Ours.
 *
 * PVH start info, version 1: the x86/HVM direct boot ABI, a public interface
 * (Xen's public/arch-x86/hvm/start_info.h, MIT, Copyright (c) 2016 Citrix
 * Systems, Inc.).  Layout reproduced; that is what an ABI is for.
 */
#ifndef AIOS_KERNEL_H
#define AIOS_KERNEL_H

#define XEN_HVM_START_MAGIC_VALUE 0x336ec578
#define PROGRAM_BASE	0x400000	/* where every program runs; its link address */

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

/* a program's heap: alloc hands out [next, end) */
struct heap {
	unsigned long next, end;
};

void kernel_say(const char *s, unsigned long n);	/* bytes to every console */
void kernel_flush(void);				/* wait until the last byte has left */
void __attribute__((noreturn)) kernel_halt(void);	/* the machine stops */
int kernel_is_ram(const struct hvm_start_info *si, unsigned long lo, unsigned long hi);

#endif
