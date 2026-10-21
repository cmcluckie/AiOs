#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# MACHINE.serial-disk@2: @1, and a kernel command line (part 20).  If AIOS_CMDLINE is
# set it is handed to the kernel as its command line (-append), the way a boot
# loader hands Linux its; PART.loader@10 gives it to a Linux program as its
# arguments.  Unset, the machine is @1 exactly.  What follows is @1's header.
#
# MACHINE.serial-disk@1: the machine petition 0005 runs on.
# MACHINE.serial-only@1 plus one device: an IDE disk on the primary channel
# (ISA, ports 0x1f0-0x1f7 and 0x3f6, the PC/AT's), the kind Minix and Linux 0.01
# ran on.  Still no PIC, so the disk can never interrupt anything; the loader polls.
# Still no timer: this machine cannot tell time, and the disk driver never tries.
# Everything else is as serial-only@1: one CPU, 64 MiB, one ISA serial port on
# stdio, no PCI, no ACPI, no RTC, no option ROMs, no NIC, no display.
#
# The disk is a file: $AIOS_DISK, or out/disk.minix.  If it does not exist the
# machine makes one, the way a disk came formatted from the shop: 1 MiB, Minix
# version 1 with 14-character names (mkfs.minix -1 -n 14), empty.  Delete it
# to start over.
# usage: machine.sh <loader.elf> <program.bin>
set -eu
disk=${AIOS_DISK:-out/disk.minix}
if [ ! -f "$disk" ]; then
	mkdir -p "$(dirname "$disk")"
	truncate -s 1M "$disk"
	mkfs.minix -1 -n 14 "$disk" > /dev/null
	echo "machine: new empty Minix disk at $disk" >&2
fi
if [ -w /dev/kvm ]; then accel="-enable-kvm -cpu host"; else accel="-accel tcg"; echo "machine: no /dev/kvm, software emulation" >&2; fi
exec qemu-system-x86_64 \
  -M microvm,x-option-roms=off,pic=off,pit=off,rtc=off,acpi=off,isa-serial=on,auto-kernel-cmdline=off \
  $accel -smp 1 -m 64M \
  -nodefaults -no-user-config -display none -nic none -monitor none -no-reboot \
  -serial stdio \
  -device isa-ide,id=ide0 \
  -drive if=none,id=disk0,file="$disk",format=raw \
  -device ide-hd,drive=disk0,bus=ide0.0 \
  -kernel "$1" -initrd "$2" ${AIOS_CMDLINE:+-append "$AIOS_CMDLINE"}
