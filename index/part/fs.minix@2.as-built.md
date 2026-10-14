# PART.fs.minix@2 as built

Hand-derived from the source on 2026-10-13, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
Everything @1 does (index/part/fs.minix@1.as-built.md), except that
minix_save writes over a file that is already there instead of refusing.  For
a name already in the root directory it reads that inode, frees every direct
zone it holds (clearing each one's bit in the zone map, the way Linux's
minix_free_block does, and zeroing the zone number), sets its size to 0, then
writes the new bytes into the first free zones exactly as for a new file, and
writes the inode back; the directory entry is left as it is.  A new name is
handled as in @1.  Files are still at most 7 KiB: seven direct zones.

## Every syscall or hardware access
None directly; whole blocks through PART.disk.ata-pio@1, two sectors each.

## Every symbol it imports
`ata_init`, `ata_read`, `ata_write`, `ata_flush` from PART.disk.ata-pio@1,
inside the image.

## Every byte it writes anywhere
To the disk only: as @1, plus, when writing over a file, one cleared bit in the
zone map for each zone the old file held, and that file's inode.

## Size
3723 bytes of code and 4 KiB of buffers in this build.
