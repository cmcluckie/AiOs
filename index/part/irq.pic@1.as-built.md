# PART.irq.pic@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).

## What it does
pic_init runs Linux's init_8259A with normal EOI: everything masked, ICW1 0x11,
the master's IRQs at vectors 0x30-0x37, the slave on IR2 at 0x38-0x3f, 8086
mode, everything masked again.  Then Linux's init_bsp_APIC: the local APIC
enabled with spurious vector 0xff and focus checking off, LVT0 set to ExtINT
so the 8259's interrupts reach the processor ("virtual wire"), LVT1 NMI and
masked.  pic_unmask clears one master line; pic_eoi sends a specific EOI (0x60
+ irq) to the master.

## Every syscall or hardware access
Ports 0x20, 0x21, 0xa0, 0xa1.  The local APIC at 0xfee00000: SPIV (0xf0, read
then written), LVT0 (0x350), LVT1 (0x360).

## Every symbol it imports
None.

## Every byte it writes anywhere
0xff, 0x11, 0x30, 0x04, 0x01 to the master; 0x11, 0x38, 0x02, 0x01 to the
slave; 0xff to both masks; the SPIV value read with its low byte replaced by
0xff and bits 8 and 9 set; 0x700 to LVT0; 0x10400 to LVT1.  Then the master
mask as lines are unmasked, and 0x60 + irq for every EOI.

## Size
3 functions, 171 bytes of code.  Source: 78 lines of code, lifted apart from
the two APIC accessors and the cached masks as plain bytes.

## What was cut
Listed in pic.c and origin.toml.
