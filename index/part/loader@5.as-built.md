# PART.loader@5 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's), `-m32 -O2 -ffreestanding`, linked static at 1 MiB.

## What it does
Everything @4 does (index/part/loader@4.as-built.md), plus files.  The load system call
hands a name, a buffer and a size to PART.fs.minix and its answer back; the save
system call hands a name and bytes.  The disk is not looked at during boot: the first
use of either system call is the first the drive hears of us, so a program that never
asks boots exactly as it did under @4, on a machine with a disk or without one.
On a machine with no disk both system calls return -1.  The clock is @4's: KVM's word
first, then the 8254 (findings 0012).  On a machine with neither, the five tries
fail at once and the ms system call says 0, as @4 promised.

## Every syscall or hardware access
As @4, plus, through PART.disk.ata-pio@1 only and only once a program has asked
for a file: ports 0x1f0-0x1f7 and 0x3f6.  55 in/out instructions in the image,
two of them rep insw and rep outsw.

## Every symbol it imports
None at link time; inside the image, `serial8250_*`, `kvmclock_khz`, `tsccal_*`, `minix_load`
and `minix_save`.

## Every byte it writes anywhere
As @4, plus what PART.fs.minix@1 and PART.disk.ata-pio@1 write, only when asked.
Nothing to the serial port of its own.

## Size
5506 bytes of code and read-only data, 8 KiB of zeroed buffers, in this build
(loader@4 built the same way is 1562).  Diff from @4: 12 lines of code in the
loader itself, two system calls appended at offsets 20 and 24, none moved.
