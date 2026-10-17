# MACHINE.serial-vga@1 as built

Written from pool/MACHINE.serial-vga@1/machine.sh and the device tree QEMU
reports for it (TST.machine.vga-only@1), 2026-10-16.

## What it does
MACHINE.serial-only@1 with a screen: one isa-vga card, and SeaBIOS
(bios-microvm.bin, shipped with QEMU) as the firmware instead of qboot, with
option ROMs on, so the card's own BIOS (vgabios.bin) runs and leaves it in 80
by 25 colour text mode with its font loaded, the way every PC since the PS/2
has done it.  SeaBIOS writes its name on the screen and leaves the blinking
cursor two lines down; the kernel clears the screen and never moves the
cursor.  No timer, no interrupt controller, no disk.

## Every syscall or hardware access
The host side: none beyond QEMU.

## Every device the guest sees
The serial port, the VGA card, and what microvm forces (isabus-bridge, ioapic,
fw_cfg_io, kvmclock or kvmvapic, and eight empty virtio-mmio transports).

## Size
One shell script.
