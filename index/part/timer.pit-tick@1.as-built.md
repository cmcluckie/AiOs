# PART.timer.pit-tick@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).

## What it does
Puts the 8254's counter 0 in mode 2 (rate generator) with a count of 11932,
Linux's PIT_LATCH for HZ 100: (1,193,182 + 50) / 100.  IRQ 0 then fires 100
times a second, from the chip, until the machine stops.  Returns HZ.

## Every syscall or hardware access
Port 0x43 (one write), port 0x40 (two writes).

## Every symbol it imports
None.

## Every byte it writes anywhere
0x34 to 0x43; 0x9c, 0x2e (11932, low byte first) to 0x40.

## Size
1 function, 27 bytes of code.  Source: 23 lines of code.

## What was cut
Listed in pittick.c and origin.toml.
