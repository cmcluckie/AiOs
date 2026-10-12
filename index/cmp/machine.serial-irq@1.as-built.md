# CMP.machine.serial-irq@1 as built

Written from pool/CMP.machine.serial-irq@1/machine.sh and the device tree QEMU
reports for it (TST.machine.irq-only@1), 2026-10-12.

## What it does
CMP.machine.serial-timer@1 plus pic=on: the PC's two 8259A interrupt
controllers (isa-i8259, or kvm-i8259 under KVM), master at 0x20, slave at
0xa0.  The 8254 (isa-pit or kvm-pit) is wired to the master's IRQ 0.  The
processor's local APIC, which every x86 processor has, comes out of qboot with
LVT0 masked, so nothing from the 8259 reaches the processor until CMP.irq.pic
sets up the virtual wire (findings 0011).  No disk.

## Every syscall or hardware access
The host side: none beyond QEMU.

## Every device the guest sees
16 in the device tree: the serial port, the 8254, the two 8259s, and what
microvm forces (isabus-bridge, ioapic, fw_cfg_io, kvmclock or kvmvapic, and
eight empty virtio-mmio transports).

## Size
One shell script.
