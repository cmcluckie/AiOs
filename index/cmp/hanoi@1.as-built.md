# CMP.hanoi@1 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`.

## What it does
Recursively enumerates the 15 moves for four disks, writing one 24-byte line
per move through the door, then calls the exit door.  Never returns.

## Every syscall or hardware access
None.  Indirect calls through the door table only.

## Every symbol it imports
None.

## Every byte it writes anywhere
360 bytes to the write door: fifteen lines of the form `Move disk N from X to Y\n`.
A 25-byte line buffer and the recursion frames on the loader's stack; recursion
depth 4.

## Size
254 bytes.
