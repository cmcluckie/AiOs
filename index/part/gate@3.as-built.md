# PART.gate@3 as built

Hand-written on 2026-11-25, pending the generator (plan 7.1).

pool/PART.gate@3/ is pool/PART.gate@2/ at 40ce2ee, copied unchanged but for
the header's first lines (findings 0050).  Everything in
index/part/gate@2.as-built.md holds for it.  What car 3 asks of it, that @2
does not do, is SPEC.gate.16@1: the sharing kernel (PART.share@2) copies each
program in its archive, byte for byte, to the bottom of its own 8 MiB of RAM
and calls gate_space_new(phys, PROGRAM_BASE, 8 MiB, PROGRAM_BASE, top), so a
Linux program file arrives there as a file, not as a program; @2 starts it at
its first byte (Guru Meditation #0000000E.00000000, findings 0049).
