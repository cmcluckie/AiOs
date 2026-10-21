# PART.cat@1 as built

Hand-derived from lifted/PART.cat@1/cat.c on 2026-10-20.  Lifted whole and
untouched from 4.4BSD-Lite (Berkeley, 1994), cat.c version 8.1 of 1993-07-19,
written by Kevin Fall; origin in lifted/PART.cat@1/origin.toml.

## What it does
Copies each file named on its command line, in order, to standard output; with no
names, or the name "-", it copies standard input.  Options: -b number the lines
that are not blank, -e show $ at each line's end, -n number every line, -s squeeze
runs of blank lines to one, -t show tabs as ^I, -u no buffering on output, -v show
characters that do not print.  -b, -e and -t turn on -n or -v as well.  An unknown
option prints the usage and exits 1.

With no options it copies raw: it asks the system how big a block standard output
likes (fstat), takes a buffer of at least 1,024 bytes (malloc), and loops read and
write until each file ends.  With any option it goes through stdio instead (fopen,
getc, putchar).

## A bug it was lifted with
It declares rval, the exit status, and never sets it.  A file it cannot open is
reported on standard error ("cat: NAME: ..."), skipped, and cat still exits 0,
which says everything worked.  Later BSDs set it to 1.  Findings 0023 item 3 and
decision 0038: it stays as lifted until its test shows it red, then it is fixed in
its own change, recorded here and in the origin record.

## On AiOs
Built by the builder against PART.libc.musl@1 and run by PART.loader@10 and
PART.gate@2 as a normal Linux program file (findings 0023 item 12): in Claude's
workspace, under software emulation, it copies what is typed, prints one file,
two, and one of 5,000 bytes, exactly; asked for a file that is not there, it says
so and still exits 0 (the bug below), so TST.cat.missing-file@1 is red.

## What it asks of the system (through musl)
Measured under strace on a normal Linux computer (findings 0023 item 4), with the
1995 version three lines apart: printing a file asks for set_thread_area,
set_tid_address, statx, open, brk, mmap2, read, write, close and exit_group; with
-v it adds ioctl, munmap and writev.  The jig measures it again on AiOs.

## Building it
It needs the older C dialect and three things BSD's own headers gave it, listed as
cflags in its catalogue entry: -std=gnu89, -D_BSD_SOURCE, -D__P(x)=x and
-include=sys/types.h (SPEC.builder.3@1).  It links against PART.libc.musl@1.

## Size
251 lines in cat.c, 199 of them code.  BSD licence, its advertising clause
deleted by the University of California's letter of 1999-07-22.
