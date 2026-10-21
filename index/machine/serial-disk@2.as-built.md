# MACHINE.serial-disk@2 as built

Hand-derived from pool/MACHINE.serial-disk@2/machine.sh on 2026-10-20.

## What it is
MACHINE.serial-disk@1 exactly (its as-built spec holds: one CPU, 64 MiB, one ISA
serial port on stdio, one IDE disk on the primary channel from $AIOS_DISK or
out/disk.minix, no PIC, timer, RTC, display, network or option ROMs), and one
thing more: when AIOS_CMDLINE is set it is handed to the kernel as its command line
(QEMU's -append), the way a boot loader hands Linux its own.  PART.loader@10 gives
it to a Linux program as its arguments.  Unset, the machine is @1 exactly.

## Devices
As @1; TST.machine.disk2-only@1 lists them (15 under software emulation in Claude's
workspace).

## Size
machine.sh 38 lines.
