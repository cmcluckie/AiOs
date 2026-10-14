# PART.timer.tsc-pit@1 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's).

## What it does
Measures how fast the processor's time stamp counter ticks.  Programs the 8254's
counter 2 for a one-shot count down from 0xffff at 1,193,182 Hz, then watches the
high byte of the count fall, reading the time stamp counter each time it
changes, until the error is under 500 parts per million or 50 ms have passed.
Returns ticks per millisecond, or 0 if the reads were not steady enough to
trust.  This is Linux's quick_pit_calibrate, unchanged in its logic.

## Every syscall or hardware access
Port 0x43 (one write: counter 2, mode 0, binary), port 0x42 (two writes, then
reads).  rdtsc.  Nothing else; not port 0x61, which Linux uses for the counter's
gate and microvm does not have.

## Every symbol it imports
None.

## Every byte it writes anywhere
0xb0 to port 0x43; 0xff, 0xff to port 0x42.

## Size
2 functions, 531 bytes of code in this build.  Source: 103 lines of code, lifted
apart from 8 lines of glue (a typedef, two #defines, a pragma, the two exported
functions); provenance in the file header.

## What was cut
Listed in tsccal.c and origin.toml.
