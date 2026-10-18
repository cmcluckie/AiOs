# PART.loader@9 as built

Hand-derived from the source on 2026-10-18, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
PART.loader@8 behind the gate.  One source; each part behind a switch the
builder sets from the flavor's petitions (HAVE_TIMER_KVMCLOCK,
HAVE_TIMER_TSC_PIT, HAVE_FS_MINIX, HAVE_SCREEN_VGA, HAVE_SHARE); PART.gate@1 is
always built in.  Given one plain program it copies it to 0x400000, gives it a
heap from its end to the end of that RAM region, and hands it to the gate
(gate_run), which runs it in ring 3; the program is handed the gate's table of
stubs, not the kernel's.  Given an archive of programs it hands them to
PART.share@2 if the flavor has it, and halts without a word if not (the
referee refuses that flavor first, rule 19).  The kernel's own table of what
to do for each system call (write, exit, alloc, read, ms, load, save) is
private to it; the gate is its only caller.  Exit flushes the console and
halts, or, with sharing, resumes the next program first.  Otherwise as @8
(clocks, disk, screen).  Differences from @8: ring 3 for every program; its
share hook is PART.share@2, not @1; the program is not called but entered by
the gate; PART.share@1 is not built with it.

## Every syscall or hardware access
As @8: the serial port through PART.serial.8250@2; rdtsc when a clock part is
in; the reset by triple fault on halt.  New: nothing of its own; the gate
holds the processor's tables and registers.

## Every symbol it imports
As @8, and gate_run (gate.h); with HAVE_SHARE, share, share_heap, share_exit
(share.h).  Exports to the parts: kernel_say, kernel_flush, kernel_halt,
kernel_is_ram (kernel.h).  Also stack_top and kernel_end (link script).

## Every byte it writes anywhere
As @8: program bytes to 0x400000 for a plain program, the serial port, its own
.data and .bss.  Plus the link script's: the stub page (4 KiB) and the gate's
stack (16 KiB, not loaded) before kernel_end.

## Size
loader.c 440 lines, loader.ld 29 (the link script adds the stub page and the
gate's stack), start.S as @8.  The kernel file (loader.elf) for the gate
flavor 20,232 bytes; for the gateshare flavor 21,028 (@8's share flavor was
15,384, per index/part/loader@8.as-built.md; the difference is the gate, whose
stub page alone is 4,096 bytes of the file).
