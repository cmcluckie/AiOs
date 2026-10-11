# CMP.life@1 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`.

## What it does
Asks the alloc door for two 24576-byte grids, seeds an R-pentomino, runs 200
generations of Conway's rules with dead edges, writes one line with the
population, 24 lines of 80 cells, then asks for 128 MiB, writes whether that was
granted, and calls the exit door.  Never returns.

## Every syscall or hardware access
None.  Indirect calls through the door table only: alloc three times, write
many times, exit once.  No `in`, `out`, no privileged instruction.

## Every symbol it imports
None.

## Every byte it writes anywhere
1992 bytes to the write door.  The two grids it was given, 49152 bytes.  An
82-byte line buffer and small frames on the loader's stack.  Its own 16 bytes
of zero-initialised data, carried in the file.

## Size
1360 bytes: code, the format strings, and 16 bytes of zeros for its pointers.
