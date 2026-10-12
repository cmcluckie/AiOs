# CMP.machine.serial-disk@1 as built

Written from pool/CMP.machine.serial-disk@1/machine.sh and the device tree QEMU
reports for it (TST.machine.disk-only@1), 2026-10-12.

## What it does
CMP.machine.serial-only@1 plus one ISA IDE channel (isa-ide: ports 0x1f0-0x1f7
and 0x3f6, IRQ 14, which goes nowhere because there is no PIC) with one hard
disk on it (ide-hd, the master).  The disk is a raw file, $AIOS_DISK or
out/disk.minix; if it does not exist the script makes it, 1 MiB, formatted with
`mkfs.minix -1 -n 14`: Minix v1, 14-character names, 352 inodes, 1024 blocks,
the first data zone at 15, empty.  No timer: pit=off.

## Every syscall or hardware access
The host side: QEMU opens the disk file read-write.  truncate and mkfs.minix
run once, when there is no disk file.

## Every device the guest sees
15 in the device tree: the serial port, the IDE channel and its disk, and what
microvm forces (isabus-bridge, ioapic, fw_cfg_io, kvmclock or kvmvapic, and
eight empty virtio-mmio transports), as for serial-only@1 (findings 0002 #8).

## Size
One shell script.
