# PART.life@2 as built

Hand-derived from the source on 2026-10-16, pending the generator (plan 7.1).

## What it does
Allocates two 24 KiB grids, puts the R-pentomino in the middle of 256 x 96,
runs 200 generations keeping the one before, then writes: reset colour, clear
screen, home; "generation 200, population N"; the middle 80 x 24 cells, '#'
bright yellow if born in the last generation, '#' green if it survived it, '.'
blue if empty, a colour escape only where the colour changes; reset colour.
Then reads one key and exits.  3075 bytes in all.

## Every syscall or hardware access
write, alloc (twice), read (once), exit.  Nothing else.

## Every symbol it imports
None: the system call table it is handed.

## Every byte it writes anywhere
Its two grids, its stack.

## Size
1440 bytes as built for loader@8's program format.
