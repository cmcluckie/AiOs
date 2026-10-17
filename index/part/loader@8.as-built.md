# PART.loader@8 as built

Hand-derived from the source on 2026-10-16, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
The kernel built from the petition (plan 5.8).  One source; each part behind a
switch the builder sets from the flavor's petitions (`HAVE_` and the part's
name: HAVE_TIMER_KVMCLOCK, HAVE_TIMER_TSC_PIT, HAVE_FS_MINIX, HAVE_SHARE).
Always: the serial console (PART.serial.8250@2), write, read, alloc, exit.

Given one plain program, it does what @5 did: copies it to 0x400000, gives it a
heap from its end to the end of that RAM region, calls it with the system call
table.  Given an archive of programs, it hands them to PART.share@1 if the
flavor has it, and halts without a word if not (the referee refuses that flavor
first, rule 19).

ms: with a clock part, the processor's counter since boot divided by its rate,
asked of KVM first (PART.timer.kvmclock@1), measured against the 8254
(PART.timer.tsc-pit@N) if KVM does not answer, as @7; 0 forever without one.
load and save: through PART.fs.minix@N, or -1 without it.

## Every syscall or hardware access
Its own: none beyond the serial port (through PART.serial.8250@2), rdtsc when a
clock part is in, and the reset by triple fault on halt (findings 0002 #6).
Everything else belongs to the parts the petitions turned on, and is in their
own as-built specs.

## Every symbol it imports
serial8250_init, _write, _poll, _getc, _flush; with the switches:
kvmclock_khz; tsccal_khz; minix_load, minix_save; share, share_heap, share_exit.
Exports to the parts: kernel_say, kernel_flush, kernel_halt, kernel_is_ram (kernel.h).

## Every byte it writes anywhere
Program bytes to 0x400000 for a plain program; the serial port; its own .data
and .bss.

## Size
As built for each flavor on it, the kernel file (loader.elf): sort 10,988 bytes
(was 11,880 on loader@5), the editor 11,372 (was 19,704 on loader@7), share
15,384 (was 19,704 on loader@6).
