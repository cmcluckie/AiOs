# PART.share@1 as built

Hand-derived from the source on 2026-10-16, pending the generator (plan 7.1).

## What it does
What PART.loader@6 and @7 did with an archive of programs
(index/part/loader@6.as-built.md, "Given an archive"), moved out of the kernel
into a part of its own and otherwise unchanged: up to four programs, 8 MiB
each from 8 MiB up, a page directory each, the 8254 tick at 100 Hz through the
8259, round robin, a Guru Meditation line for a program that faults.  It
reaches the rest of the kernel through kernel.h: the console (kernel_say,
kernel_flush), the halt, and the firmware's memory map (kernel_is_ram).

## Every syscall or hardware access
The 8259s and the local APIC through PART.irq.pic@1; the 8254's mode register
and counter 0 through PART.timer.pit-tick@1; cr0, cr3, cr4 and the IDT itself.

## Every symbol it imports
pic_init, pic_unmask, pic_eoi; pit_tick_start; find_cpio_data; kernel_say,
kernel_flush, kernel_halt, kernel_is_ram; kernel_end (the link script).

## Every byte it writes anywhere
Each program's region; four page directories and one page table; the IDT; each
program's saved registers on its own stack.

## Size
share.c 267 lines, isr.S 86, as moved.
