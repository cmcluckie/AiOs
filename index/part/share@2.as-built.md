# PART.share@2 as built

Hand-derived from the source on 2026-10-18, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
What PART.share@1 did, behind the gate (PART.gate@1) and with its page tables,
interrupt table and register saving moved there.  What is left is the turns:
up to four programs from a cpio archive, 8 MiB of RAM each from 8 MiB up, the
same virtual window for all (0x400000 to 0xbfffff, stack at the top, heap
between), a space from the gate for each.  The 8254's counter 0 interrupts 100
times a second through the 8259, and on each tick the next program that is
still running gets the processor (round robin, preemptive).  When a program
exits, or the processor stops it, the next one runs; a stopped program gets
"Guru Meditation #VVVVVVVV.AAAAAAAA  program N stopped" on the console and the
others carry on; when the last has gone the machine halts.  A spurious
interrupt (the 8259's IRQ 7, the APIC's 0xff) changes nothing and is not
acknowledged.  A fault in the kernel itself is not handled here: the gate
halts.  Changes from @1: ring 3, a kernel stack for each program, no
page-table or interrupt-table code of its own.

## Every syscall or hardware access
The 8259s and the local APIC through PART.irq.pic@1 (pic_init, pic_unmask,
pic_eoi); the 8254's counter 0 through PART.timer.pit-tick@1
(pit_tick_start).  The processor's registers only through the gate.

## Every symbol it imports
pic_init, pic_unmask, pic_eoi; pit_tick_start; find_cpio_data; gate_share_calls,
gate_schedule, gate_space_new, gate_start, gate_resume; kernel_say, kernel_flush,
kernel_halt, kernel_is_ram.  Exports: share, share_heap, share_exit (share.h).

## Every byte it writes anywhere
Each program's region (the program's bytes, copied from the archive); its own
task table (four entries) and the one message buffer.  Everything else it
causes is written by the gate.

## Size
share.c 177 lines, share.h 20 (@1 had 267 and 86 in isr.S, 20 in share.h).
