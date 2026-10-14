# PART.stopwatch@1 as built

Hand-derived from the source on 2026-10-11, pending the generator (plan 7.1).

## What it does
Notes what the ms system call says at start, then for n = 0, 1, 2 ... waits until the ms system call says
start + n * 1000 and writes "\rMM:SS" for n seconds (minutes wrap at 100).  Never
exits: a stopwatch runs until the machine is switched off.  Waiting is making
the ms system call in a loop; nothing else.

## Every syscall or hardware access
The write system call and the ms system call.  No hardware.

## Every symbol it imports
None; the system call table is handed to it.

## Every byte it writes anywhere
Six bytes a second to the write system call: carriage return, two digits, colon, two
digits.

## Size
208 bytes as a flat binary in this build.  Source: 23 lines of code.
