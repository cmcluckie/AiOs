# PART.loader@3 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`,
linked static at 1 MiB.

## What it does
Everything @2 does (index/part/loader@2.as-built.md), plus the read system call: waits
for one byte on the serial port, then takes any others already waiting, up to
the caller's limit.  Polling only; interrupts stay off.  A program that never
calls read never touches the receive side.

## Every syscall or hardware access
As @2, plus reads of the serial port's line status and receive registers,
through PART.serial.8250@2 only.

## Every symbol it imports
None at link time; `serial8250_*` from PART.serial.8250@2 inside the image.

## Every byte it writes anywhere
As @2, plus received bytes into the caller's buffer.

## Size
922 bytes of code and read-only data (up from 742).  Diff from @2: 13 lines of
code, one system call appended, none moved.
