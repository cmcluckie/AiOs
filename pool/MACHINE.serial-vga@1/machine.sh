#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# MACHINE.serial-vga@1: the machine petition 0009 runs on.
# MACHINE.serial-only@1 plus a screen: one ISA VGA card (isa-vga), the kind
# every PC has had since IBM's PS/2 in 1987, in its 80 by 25 colour text mode
# with the character cells at 0xb8000.  A VGA card comes up blank; on a real PC
# its own BIOS, run by the PC's BIOS, puts it in text mode and loads the font.
# So does this one: the firmware is SeaBIOS (bios-microvm.bin, QEMU's own)
# instead of qboot, because qboot does not run a card's BIOS, and option ROMs
# are on so the card's BIOS (vgabios.bin) runs.  The firmware writes its name
# on the screen before the kernel starts; the kernel clears it.  Everything
# else is as serial-only@1: one CPU, 64 MiB, one ISA serial port on stdio, no
# PCI, no ACPI, no RTC, no timer, no interrupt controller, no NIC, no disk.
# The screen is shown nowhere (-display none); the tester reads it with QEMU's
# screendump, pixels, not bytes.
# usage: machine.sh <loader.elf> <program.bin>
set -eu
if [ -w /dev/kvm ]; then accel="-enable-kvm -cpu host"; else accel="-accel tcg"; echo "machine: no /dev/kvm, software emulation" >&2; fi
exec qemu-system-x86_64 \
  -M microvm,x-option-roms=on,pic=off,pit=off,rtc=off,acpi=off,isa-serial=on,auto-kernel-cmdline=off \
  -bios bios-microvm.bin -device isa-vga \
  $accel -smp 1 -m 64M \
  -nodefaults -no-user-config -display none -nic none -monitor none -no-reboot \
  -serial stdio \
  -kernel "$1" -initrd "$2"
