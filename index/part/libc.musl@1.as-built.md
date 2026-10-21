# PART.libc.musl@1 as built

Hand-derived from lifted/PART.libc.musl@1/ and its origin record on 2026-10-20.

## What it is
The part of musl 1.2.4, the C library, that Berkeley's cat takes, lifted whole and
untouched at each file's own path in musl's tree (origin record: into = ".").
Which files: the source files the linker pulled in when cat was linked against all
of musl on the jig (findings/0023-pieces.txt, by jig/libc pieces), every header
those files and cat include, the four inputs musl's build turns into its two
generated headers (bits/alltypes.h and bits/syscall.h), and musl's licence,
COPYRIGHT.  Nothing else of musl is here.

## What is in it
- 82 source files: 78 in C and 4 in 386 assembly (memcpy, memset, __set_thread_area
  and defsysinfo), under src/:
  starting a program (__libc_start_main, __init_tls, __set_thread_area), stdio
  (fopen, fclose, getc, putc, putchar, fprintf, vfprintf, fwrite, fflush, setbuf
  and what they share), memory (lite_malloc, mallocng's malloc and free, mmap,
  munmap, mprotect), files (open, read, write, close, lseek, fstat, fstatat),
  getopt, err and warn, strerror, exit, the system call wrapper and its errno.
- 87 headers: musl's public ones under include/, its internal ones under
  src/internal/ and src/include/, and the 386's under arch/i386/ and arch/generic/.
- The generators: arch/i386/bits/alltypes.h.in, include/alltypes.h.in,
  tools/mkalltypes.sed and arch/i386/bits/syscall.h.in.

## What it needs
A 64-bit divide helper the compiler would normally supply: PART.divide64@1.  The
system calls listed in PART.cat@1's as-built spec, at Linux's 386 numbers, which
AiOs does not all answer yet.

## Size
174 files, 15,253 lines: 4,245 in the 82 source files and the 4 generators and
COPYRIGHT, 11,008 in the 87 headers.  11,869 code lines in the .c and .h files
(no blanks, no comments).  MIT licence.
