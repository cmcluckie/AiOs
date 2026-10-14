# PART.fs.minix@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).  Measured with the gcc in Claude's workspace (not the
jig's).

## What it does
Reads and writes the original Minix filesystem: version 1, 1 KiB blocks,
14-character names, magic 0x137F.  On the first call it reads the superblock
(block 1), refuses anything but that version, and checks it the way Linux's
minix_check_superblock does.  minix_load finds a name in the root directory
(inode 1) and copies up to seven blocks of the file out.  minix_save refuses a
name that is already there or longer than 14 characters, or more than 7 KiB;
otherwise it takes the first free inode from the inode map, the first free
zones from the zone map, writes the data, writes the inode (mode 0100644, owner
0, one link, dated 0: 1 January 1970), adds the name to the first empty slot in
the root directory or at its end, growing it if it has to, and flushes the
drive.  Every block is read, changed and written straight back: no cache, no
mount, no unmount.  Failures are -1, and a save that fails halfway leaves the
bits it set marked busy.

## Every syscall or hardware access
None directly; whole blocks through PART.disk.ata-pio@1, two sectors each.

## Every symbol it imports
`ata_init`, `ata_read`, `ata_write`, `ata_flush` from PART.disk.ata-pio@1,
inside the image.

## Every byte it writes anywhere
To the disk only: one bit in the inode map and one per zone in the zone map,
the new inode's 32 bytes, the file's blocks (the last padded with zeros), the
16-byte directory entry, and the root directory's inode when its size changes.
Nothing else on the disk is written.

## Size
3 exported functions, 3335 bytes of code and 4 KiB of buffers in this build.
Source: 344 lines of code, about 120 of them glue of ours (block reads and
writes, the bitmap bit helpers, copying instead of memcpy, and minix_load and
minix_save, which call Linux's pieces in Linux's order); provenance in the file
header and origin.toml.

## What was cut
Listed in minix.c and origin.toml.
