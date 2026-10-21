# TOOL.libc@1 as built

Hand-derived from jig/libc on 2026-10-20, pending the generator (plan 7.1).
A scaffold: a shell script standing in for the builder's C library step.

## What it does
Builds musl, the C library, for 32-bit x86, from the one commit in jig/musl.pin
(v1.2.4), with plain sh and no make: the rules of musl's own Makefile, written
out.  Then, given a program's object files, it says which of musl's source files
that program takes.  It decides nothing and copies nothing into the repo.

## The commands
- build: the source comes out of a clone of musl (MUSL, default ../musl) at the
  pin, through git archive.  It makes the two generated headers the way the
  Makefile does (bits/alltypes.h, bits/syscall.h), then compiles every file in
  src/*, src/malloc/mallocng and crt (crt1, crti, crtn only; the start files for
  shared and position-independent programs are skipped), where a file in a
  directory's i386/ replaces the general one of the same name.  The flags are
  musl's own for i386 (configure's output for --target=i386), with warnings off.
  The library goes to out/musl/lib/libc.a with each piece named by its whole
  path, / as @ (src/stdio/fopen.o is src@stdio@fopen.o), so src/malloc/free.c and
  src/malloc/mallocng/free.c stay two pieces.  The headers a program sees go to
  out/musl/include, the start files to out/musl/lib, and the list of pieces and
  their sources to out/musl/sources.txt.
- pieces OBJ...: links those objects with the start files and libc.a, statically,
  with a link map, and prints the source file of every piece the linker pulled
  in, one to a line, sorted.

## Refusals
Exit 1: no build yet; a file that does not compile; a program that does not link
(the linker's complaint is printed); a piece with no known source.  Exit 2: no
commit in jig/musl.pin, no clone, the clone lacks the pinned commit, no ar.
MUSL_TREE=/dir builds from an unpacked tree instead, for testing only.

## Everything it touches
- Reads: jig/musl.pin; the clone at MUSL through git cat-file and archive.
- Writes: out/musl (LIBC_OUT moves it), and a scratch folder, removed on exit.
- Calls: the compiler (CC, default gcc), ar, the linker through the compiler.
- No network, no ports, no registers.

## Size
99 lines in jig/libc (shell).  Needs git, tar, sed, awk, ar, a C compiler that can build -m32.
