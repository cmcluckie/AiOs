# PART.sort@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).

## What it does
Makes twenty numbers with the rand() printed in Kernighan and Ritchie, section
2.7, from seed 1, each mod 1000.  Sorts them by insertion.  Prints "sorted 20
numbers: " and the line.  Asks for "sorted": if there is none, it
saves the line under that name and says how many bytes; if there is one, it says
how many bytes it found and whether they are the line it just printed, and
writes nothing.  Then exits.

## Every syscall or hardware access
None.  System calls: write, load, save, exit.

## Every symbol it imports
None; the system call table is handed to _start.

## Every byte it writes anywhere
To the console: the two lines above (137 bytes on a fresh disk, 143 after).  To
the disk, through the save system call, once, on a disk without the file: the 74-byte
line.

## Size
1280 bytes, flat binary.
