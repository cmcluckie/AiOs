#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# CMP.machine.serial-irq@1: the machine petition 0006 runs on.
# CMP.machine.serial-timer@1 plus the device that lets the timer interrupt: the
# PC's pair of 8259A interrupt controllers (pic=on).  The 8254's counter 0
# ticks through the 8259 to the processor, and that tick is what lets the
# kernel take the processor back from a program.  Counter 2 is still measured
# against at boot, as for the stopwatch.  Everything else is as serial-only@1:
# one CPU, 64 MiB, one ISA serial port on stdio, no PCI, no ACPI, no RTC, no
# option ROMs, no NIC, no disk, no display.
# usage: machine.sh <loader.elf> <program.bin or archive of programs>
set -eu
if [ -w /dev/kvm ]; then accel="-enable-kvm -cpu host"; else accel="-accel tcg"; echo "machine: no /dev/kvm, software emulation" >&2; fi
exec qemu-system-x86_64 \
  -M microvm,x-option-roms=off,pic=on,pit=on,rtc=off,acpi=off,isa-serial=on,auto-kernel-cmdline=off \
  $accel -smp 1 -m 64M \
  -nodefaults -no-user-config -display none -nic none -monitor none -no-reboot \
  -serial stdio \
  -kernel "$1" -initrd "$2"
