# MACHINE.serial-timer@1 as built

Hand-derived from pool/MACHINE.serial-timer@1/machine.sh and QEMU's own
device tree (`info qtree` on a machine started with -S) on 2026-10-11, pending
the generator (plan 7.1).

## What it does
MACHINE.serial-only@1 with one change: pit=on.  QEMU's 8254 programmable
interval timer is present at ports 0x40 to 0x43.  The PIC is still off, so the
timer is wired to nothing and can interrupt nothing.  Everything else as
serial-only@1: one CPU, 64 MiB, one ISA serial port on stdio, no RTC, no ACPI, no
option ROMs, no network, disk or display, -no-reboot.

## Every device the guest can see
isa-serial at 0x3f8; isa-pit at 0x40 (kvm-pit under KVM); isabus-bridge,
ioapic, fw_cfg_io, kvmvapic, and eight empty virtio-mmio transports.  14 in all
under software emulation.  No port 0x61: there is no PC speaker device, so the
8254's counter 2 gate cannot be switched (findings 0008).

## On the jig
The jig runs it under KVM inside Hyper-V, so every read of the 8254 crosses two
hypervisors.  Measured with tests/programs/pitprobe.c: on the jig, 12 to 24
reads in each 214.6 us step of the counter, and 66,000 to 129,000 counter
(TSC) cycles for one pair of reads; under software emulation in Claude's
workspace, 460 to 1,130 reads a step and about 850 cycles a pair (findings
0012 item 3).

## Firmware
qboot, as serial-only@1.  It never writes to the serial port.

## Every byte it writes anywhere
None of its own.

## Size
One shell script, 17 lines.
