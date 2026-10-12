# CMP.disk.ata-pio@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's).

## What it does
Talks to the first IDE disk on the PC/AT's primary channel.  ata_init turns the
drive's interrupt off, selects the master, waits until it is not busy, and says
whether anyone answered: a status of 0xff means no controller, a status without
"ready" means no drive.  ata_read and ata_write move one 512-byte sector by its
number (LBA28): wait idle, load the address into the task file, send the
command, wait while busy, check for an error or a fault, then move 256 16-bit
words with rep insw or rep outsw.  ata_flush asks the drive to write its own
cache out.  Every wait is on the drive's busy bit, as long as it takes; there is
no timeout, because there is no clock (findings 0010).  Nothing is cached.

## Every syscall or hardware access
Ports 0x1f0-0x1f7 (data, features, sector count, the three address bytes,
device, command and status) and 0x3f6 (device control on write, alternate
status on read).  No IRQ, no DMA.

## Every symbol it imports
None.

## Every byte it writes anywhere
0x02 to 0x3f6 (interrupts off at the drive).  0xa0 to 0x1f6 to select the
master; 0xe0 plus the top four bits of the sector number with every command.
0x00 to 0x1f1, 0x01 to 0x1f2, the sector number's low three bytes to 0x1f3-0x1f5,
then 0x20 (read), 0x30 (write) or 0xe7 (flush) to 0x1f7.  The 512 bytes of a
sector being written, to 0x1f0.

## Size
4 exported functions, 542 bytes of code in this build.  Source: 122 lines of
code; provenance in the file header and origin.toml.

## What was cut
Listed in ata.c and origin.toml.
