# CMP.stopwatch@1 as built

Hand-derived from the source on 2026-10-11, pending the generator (plan 7.1).

## What it does
Notes the ms door at start, then for n = 0, 1, 2 ... waits until the door says
start + n * 1000 and writes "\rMM:SS" for n seconds (minutes wrap at 100).  Never
exits: a stopwatch runs until the machine is switched off.  Waiting is asking
the door in a loop; nothing else.

## Every syscall or hardware access
The write door and the ms door.  No hardware.

## Every symbol it imports
None; the door is handed to it.

## Every byte it writes anywhere
Six bytes a second to the write door: carriage return, two digits, colon, two
digits.

## Size
208 bytes as a flat binary in this build.  Source: 23 lines of code.
