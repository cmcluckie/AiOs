# PART.loader@6 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's), `-m32 -O2 -ffreestanding`, linked static at 1 MiB.

## What it does
Given one plain program, exactly what @5 does (index/part/loader@5.as-built.md):
no interrupt controller, no timer tick, no page tables.

Given an archive of programs (a newc cpio, magic 070701), the kernel:
- walks the archive with PART.archive.cpio and gives each program, up to four,
  its own 8 MiB of RAM from 8 MiB up (8-16 MiB, 16-24 MiB...), refusing any
  that would not fit in RAM by the firmware's map;
- copies each program to the bottom of its region and builds it a page
  directory: the kernel's own pages (1 MiB to kernel_end, 4 KiB pages) the same
  in every directory, and the program's region as two 4 MiB pages at 0x400000,
  where every program on the ladder is linked.  Nothing else is mapped: not page
  0, not another program's region;
- gives each program a 16 KiB stack at the top of its region and a heap from
  the end of its image to the stack; alloc hands out only that;
- loads an IDT with gates for the processor's 32 exceptions, IRQ 0 (vector
  0x30), the 8259's spurious IRQ 7 (0x37) and the APIC's spurious vector (0xff);
- sets up the 8259s (PART.irq.pic), starts the 8254's counter 0 at 100 Hz
  (PART.timer.pit-tick), unmasks IRQ 0 only, turns paging on, and enters the
  first program;
- on every tick, saves the running program's registers on its own stack and
  resumes the next live one, round robin, switching page directory;
- on any exception, prints `Guru Meditation #VVVVVVVV.AAAAAAAA  program N
  stopped` (the vector, then the faulting address for a page fault or the
  instruction's address otherwise), stops that program alone and resumes the
  next; when no program is left, halts;
- on exit, stops the caller and resumes the next; the last exit halts.
Every system call runs with interrupts off, so a tick never splits a system call;
waiting in read lets the others run.

## Every syscall or hardware access
As @5, plus, only with an archive: the 8259s (ports 0x20, 0x21, 0xa0, 0xa1)
and the local APIC (0xfee000f0, 0xfee00350, 0xfee00360) through
PART.irq.pic@1; the 8254's mode register and counter 0 (0x43, 0x40) through
PART.timer.pit-tick@1; cr0, cr3, cr4 (PSE, PG), cr2 on a page fault, lidt.

## Every symbol it imports
None at link time; inside the image, everything @5 used plus `pic_*`,
`pit_tick_start` and `find_cpio_data`.

## Every byte it writes anywhere
As @5.  With an archive, the programs' copies and stacks in their regions, the
page directories and table in its own memory, and to the console only the Guru
Meditation line when a program faults.

## Size
8357 bytes of code and read-only data, 36 KiB of zeroed tables and buffers, in
this build (loader@5 built the same way is 5506).  Diff from @5: 350 lines of
code against 134, and isr.S, 67 lines.  The system call table is unchanged.

## Not yet
Programs run at the kernel's privilege (ring 0).  One could write to the
kernel's own pages, which every directory maps writable, or turn paging off.
Findings 0011.
