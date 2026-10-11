# CMP.loader@1 as built

Hand-derived from the source and the disassembly on 2026-10-10, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`,
linked static at 1 MiB.

## What it does
Entered by the machine's firmware under the x86/HVM direct boot ABI in 32-bit
protected mode.  Loads a three-entry GDT, reloads segments, sets a 4 KiB stack,
initialises the serial segment, checks the start-info magic, copies module 0 to
0x400000, calls it with the door.  On exit: drains the serial port, loads a null
IDT and executes `int3`, which triple-faults; the machine treats that as a reset
and, with `-no-reboot`, exits.

## Every syscall or hardware access
`lgdt` once, `lidt` twice (one per exit path), `int3`, `cli`, `hlt`, `ljmp`.
Reads the start-info structure and module 0 wherever the firmware placed them.
Writes memory from 0x400000 for the module's length.  The serial port, only
through CMP.serial.8250@1.  Nothing else: no PIC, no PIT, no APIC, no fw_cfg, no
page tables, no MSRs.

## Every symbol it imports
None at link time; `serial8250_*` resolve from CMP.serial.8250@1 inside the same image.

## Every byte it writes anywhere
The program's bytes to 0x400000.  The program's output bytes to the serial segment.
Its own stack.

## Size
ELF 5540 bytes; 572 bytes of code and read-only data (loader plus serial segment),
4096 of stack.  Source: 41 lines of assembly, 57 of C, 8 of header.
