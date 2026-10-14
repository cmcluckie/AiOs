# PART.archive.cpio@1 as built

Hand-derived from the source and the disassembly on 2026-10-12, pending the
generator (plan 7.1).

## What it does
Linux's find_cpio_data: from a pointer into a newc cpio archive (magic 070701
or 070702), skips zero padding, parses each 110-byte header of hexadecimal
fields, and returns the first regular file whose name starts with the given
path, with its data address, size and name (cut to 17 characters), and the
offset of the header after it.  Stops, returning nothing, at a bad digit, a bad
magic or a member that runs past the end; the trailer is not a regular file, so
a walk ends there.

## Every syscall or hardware access
None.  Reads the archive; writes nothing but the returned name.

## Every symbol it imports
None.

## Every byte it writes anywhere
The returned struct.

## Size
1 exported function, 559 bytes of code.  Source: 105 lines of code, about 30 of
them glue (strlen, memcmp, strscpy, PTR_ALIGN).
