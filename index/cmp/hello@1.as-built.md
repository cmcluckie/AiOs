# CMP.hello@1 as built

Hand-derived from the source and the disassembly on 2026-10-10, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`.

## What it does
Calls the door's write with 14 bytes, then the door's exit with 0.  Never returns.

## Every syscall or hardware access
None.  Two indirect calls through the door table.  No `in`, `out`, no privileged
instruction, no memory outside its own 43 bytes and the stack the loader lent it.

## Every symbol it imports
None.  The door arrives as a pointer argument.

## Every byte it writes anywhere
The 14 bytes `Hello, world.\n` to the write door.  Nothing else.

## Size
43 bytes: 27 of code, 15 of string (the NUL terminator is present but not written), 1 of padding.
