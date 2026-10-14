# PART.loader@4 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's), `-m32 -O2 -ffreestanding`, linked static at 1 MiB.

## What it does
Everything @3 does (index/part/loader@3.as-built.md), plus the clock.  At boot,
after starting the serial port, it asks KVM through PART.timer.kvmclock how
fast the processor's counter ticks; with no KVM to ask, it measures through
PART.timer.tsc-pit, up to five tries of up to 150 ms each.  Then it notes the
counter.  The ms system call then returns (counter now - counter at boot) / ticks per
millisecond: one subtraction and one division, nothing added up, so nothing
drifts.  If no try succeeded, the ms system call returns 0 forever.  After 49 days and 17
hours it holds at 0xffffffff.  Programs never see ticks.

## Every syscall or hardware access
As @3, plus, during boot only: CPUID leaves 1, 0x40000000 up to 0x4000ff00 and
0x40000001, and MSR 0x4b564d01 written twice (lend 32 bytes, take them back),
through PART.timer.kvmclock@1, under KVM; and, when that says 0, through
PART.timer.tsc-pit@2 only: the 8254's
mode register (port 0x43, one write per try) and counter 2 (port 0x42, two
writes and repeated reads per try).  The processor's time stamp counter, read
with rdtsc, at boot and on every ms system call.  32 in/out instructions
in the image: 17 on the serial port, 15 on the 8254.

## Every symbol it imports
None at link time; `serial8250_*` from PART.serial.8250@2 and `tsccal_khz`,
`tsccal_cycles` from PART.timer.tsc-pit@2, `kvmclock_khz` from
PART.timer.kvmclock@1, inside the image.

## Every byte it writes anywhere
As @3, plus the 8254 programming bytes above, and under KVM the two MSR
values: the address of 32 bytes in its own memory with bit 0 set, then 0.  Nothing to the serial port of its
own: the boot stays silent.

## Size
1562 bytes of code and read-only data in this build (the loader@3 built the same
way is 902).  Diff from @3: 22 lines of code, one system call appended at offset
16, none moved.
