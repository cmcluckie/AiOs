# PART.sieve@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).

## What it does
Makes the ms system call, then runs BYTE's 1981 sieve 800 times without a system call:
8191 flags set, then for each still set, its prime (2i + 3) strikes out its
multiples, 1899 primes a run.  Makes the ms system call again and prints
"\nBYTE sieve: 1899 primes, 800 times over, in N ms\n", then exits.

## Every syscall or hardware access
None.  System calls: ms (twice), write (once), exit.

## Every symbol it imports
None; the system call table is handed to _start.

## Every byte it writes anywhere
Its own flags; the one line to the console.

## Size
8832 bytes, flat binary, 8191 of them the flags written out as zeros.
