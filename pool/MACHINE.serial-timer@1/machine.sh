#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# MACHINE.serial-timer@1: the machine petition 0004 runs on.
# MACHINE.serial-only@1 plus one device: the 8254 programmable interval
# timer (pit=on), so the loader can measure how fast the processor's counter
# ticks.  Still no PIC, so the timer can never interrupt anything; the loader
# reads its counter for up to 150 ms at boot and never touches it again.
# Everything else is as serial-only@1: one CPU, 64 MiB, one ISA serial port on
# stdio, no PCI, no ACPI, no RTC, no option ROMs, no NIC, no disk, no display.
# usage: machine.sh <loader.elf> <program.bin>
set -eu
if [ -w /dev/kvm ]; then accel="-enable-kvm -cpu host"; else accel="-accel tcg"; echo "machine: no /dev/kvm, software emulation" >&2; fi
exec qemu-system-x86_64 \
  -M microvm,x-option-roms=off,pic=off,pit=on,rtc=off,acpi=off,isa-serial=on,auto-kernel-cmdline=off \
  $accel -smp 1 -m 64M \
  -nodefaults -no-user-config -display none -nic none -monitor none -no-reboot \
  -serial stdio \
  -kernel "$1" -initrd "$2"
