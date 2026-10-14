# PART.loader@2 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`,
linked static at 1 MiB.

## What it does
Everything @1 does (index/part/loader@1.as-built.md), plus: after copying the
program, finds the RAM region of the firmware's memory map that contains the
program's address and marks everything from the program's end to that region's
end as the heap.  The alloc system call hands out 16-byte-aligned blocks from a bump
pointer and returns 0 when the request would pass the region's end, for a
request of 0, or on overflow.  Nothing is freed.

## Every syscall or hardware access
As @1, plus reads of the memory map table wherever the firmware placed it.
Writes to memory only where the program asked and only inside the chosen
region.  No new port, register or instruction.

## Every symbol it imports
None at link time; `serial8250_*` from PART.serial.8250@1 inside the image.

## Every byte it writes anywhere
As @1, plus the program's bytes to 0x400000 for the module's length (now
including the program's zero-initialised data, carried in the file).  The
heap is written only by the program.

## Size
ELF 5540 bytes; 742 bytes of code and read-only data (up from 572), 4104 of
stack and heap bookkeeping.  Diff from @1: 35 lines of code added, one entry
appended to the system call table, no entry moved.
