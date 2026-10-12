#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# CMP.machine.serial-only@1: the machine petition 0000 runs on.
# One CPU, 64 MiB, one ISA serial port on stdio, and as little else as microvm
# allows: no PCI, no ACPI (which also selects qboot, the firmware that never
# writes to the serial port), no PIC, no PIT, no RTC, no option ROMs, no NIC, no
# disk, no display, no monitor.  What it still forces is in findings 0002 #8.
# The loader is passed in as the ELF "kernel"; the program as the first module
# (-initrd).  A reset request ends the process (-no-reboot), so "halt" is the
# QEMU process exiting.
# usage: machine.sh <loader.elf> <program.bin>
set -eu
# KVM is the machine.  Without it (WSL, a container) fall back to software
# emulation and say so on stderr; a run that way is evidence, not the done-when.
if [ -w /dev/kvm ]; then accel="-enable-kvm -cpu host"; else accel="-accel tcg"; echo "machine: no /dev/kvm, software emulation" >&2; fi
exec qemu-system-x86_64 \
  -M microvm,x-option-roms=off,pic=off,pit=off,rtc=off,acpi=off,isa-serial=on,auto-kernel-cmdline=off \
  $accel -smp 1 -m 64M \
  -nodefaults -no-user-config -display none -nic none -monitor none -no-reboot \
  -serial stdio \
  -kernel "$1" -initrd "$2"
