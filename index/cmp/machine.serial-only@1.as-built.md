# CMP.machine.serial-only@1 as built

Hand-derived from pool/CMP.machine.serial-only@1/machine.sh and QEMU's own
device tree (`info qtree` on a machine started with -S) on 2026-10-11, pending
the generator (plan 7.1).

## What it does
Starts QEMU's microvm machine type with one CPU, 64 MiB of RAM, the PIC, PIT,
RTC, ACPI and option ROMs switched off, one ISA serial port wired to stdio, no
network device, no disk, no display and no monitor.  Loads the loader as the
kernel and the program as the first module.  `-no-reboot` turns the guest's
reset into the process exiting.  Uses KVM when /dev/kvm is writable, and
otherwise software emulation, saying so on stderr.

## Every device the guest can see
isa-serial at 0x3f8 (the one the petition asks for); isabus-bridge, ioapic,
fw_cfg_io, kvmvapic, and eight empty virtio-mmio transports, which microvm
cannot omit (findings 0002 #8).  No network device of any kind.

## Firmware
qboot, selected by acpi=off.  It never writes to the serial port.

## Every byte it writes anywhere
None of its own.  Whatever the guest writes to the serial port goes to stdout.

## Size
One shell script, 21 lines.
