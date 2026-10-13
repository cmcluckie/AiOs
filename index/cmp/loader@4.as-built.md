# CMP.loader@4 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's), `-m32 -O2 -ffreestanding`, linked static at 1 MiB.

## What it does
Everything @3 does (index/cmp/loader@3.as-built.md), plus the clock.  At boot,
after starting the serial port, it asks CMP.timer.tsc-pit how fast the
processor's counter ticks, up to five tries of up to 150 ms each, and notes the
counter.  The ms door then returns (counter now - counter at boot) / ticks per
millisecond: one subtraction and one division, nothing added up, so nothing
drifts.  If no try succeeded, the door returns 0 forever.  After 49 days and 17
hours it holds at 0xffffffff.  Programs never see ticks.

## Every syscall or hardware access
As @3, plus, through CMP.timer.tsc-pit@2 only and only during boot: the 8254's
mode register (port 0x43, one write per try) and counter 2 (port 0x42, two
writes and repeated reads per try).  The processor's time stamp counter, read
with rdtsc, at boot and on every call to the ms door.  32 in/out instructions
in the image: 17 on the serial port, 15 on the 8254.

## Every symbol it imports
None at link time; `serial8250_*` from CMP.serial.8250@2 and `tsccal_khz`,
`tsccal_cycles` from CMP.timer.tsc-pit@2 inside the image.

## Every byte it writes anywhere
As @3, plus the 8254 programming bytes above.  Nothing to the serial port of its
own: the boot stays silent.

## Size
1562 bytes of code and read-only data in this build (the loader@3 built the same
way is 902).  Diff from @3: 22 lines of code, one door entry appended at offset
16, none moved.
