# TOOL.builder@1 as built

Hand-derived from jig/aios on 2026-10-20, brought up to date the same day (a program's own compile flags; who decided each petition), pending the generator (plan 7.1).
A scaffold: a shell script standing in for the builder.  Only the part that
part 19 changed is gone through closely; the rest is said in a line.

## What it does
Turns a flavor into a loader, its programs and an image, calling the referee
before and after.  When it cannot finish because the petitions name catalogue
numbers that have no entry, it stops and writes those numbers down.

## The commands
- list: the flavors and petitions in the repo.
- missing FLAVOR: the catalogue numbers named by the flavor's petitions that have
  no entry, one to a line, sorted, as "NUMBER  KIND not in the catalogue", and
  nothing else.  Nothing printed when nothing is missing.
- build FLAVOR: first, every petition the flavor names must be decided (below);
  if one is not, exit 1, naming it, before anything else.  Then the same list.  If it is not empty: it is written to
  out/FLAVOR/missing.txt, a line saying how many are missing goes to stderr, and
  the exit is 3; nothing is compiled.  If it is empty: any old missing.txt is
  removed, every program's extra compile flags are checked (below), the
  referee checks the flavor, then the loader and each program are compiled and linked and an image is made.  A program that depends on nothing
  is refused (rule 17), a source not in the pool is refused (rule 18), and the
  referee's hardware check (rule 12) runs on each program after it is built.
  The kernel and its parts are compiled with -DGRANT_ and the name of every
  capability the flavor's petitions ask for (CAP.fs.write@2 is GRANT_FS_WRITE),
  and as soon as the kernel is linked the referee holds it to rule 11
  (`referee artifact`); a refusal stops the build before any program is
  compiled (decision 0065).
- run FLAVOR: build, then boot on the flavor's machine.
- pack ARCHIVE NAME FILE...: programs into one cpio archive, in order.

## Who decided each petition
A flavor is built only when every petition it names appears, by its exact file
(petitions/NNNN.toml), in the touches of a decision in decisions/ whose by is a
key in constitution/keys/ (SPEC.builder.4, decision 0051).  A decision by a key
that is not there, one naming another petition, or one naming petitions/*
decides nothing; touches are read with globbing off, so petitions/* is not turned
into the files that exist.  Petitions 0000 to 0014 are decided by decisions 0052
to 0054.

## A program's own compile flags
A program's catalogue entry may list `cflags`: extra flags for that program's
compile only, added after the builder's own, in the order listed (part 20; old
programs such as Berkeley's cat need them).  Three forms are allowed: -std= with
a dialect name, -D with a macro name (with or without one argument, and a value),
and -include= with a header path, handed to the compiler as -include and the
path.  Anything else stops the build before anything is compiled, naming the
program and the flag.  The referee's look at the program's own instructions
(rule 12) is compiled with the same flags, each in quotes, since the referee hands
them to a shell.

## A normal Linux program
A program whose catalogue entry depends on PART.libc.musl@1 is built the way a Linux
toolchain builds a static program (part 20): musl is built once per flavor from
lifted/PART.libc.musl@1 (the two generated headers from their lifted inputs, every
lifted source with musl's own flags for i386, into a library; its three start files;
the headers a program sees), with PART.divide64@1; the program is compiled for the 486 (-march=i486, as musl is; findings 0023
item 13) against those headers with its own flags, looked at by the referee (rule 12) with the same,
and linked statically, not position-independent, at the address the linker
chooses (0x08048000 and up): an ELF file, out/FLAVOR/NAME.bin.

## Refusals
Exit 1 with "refused:" and the rule or reason.  Exit 3 only for the missing list.
Exit 2 for an unknown command.

## Everything it touches
- Reads: flavors/, petitions/, decisions/ and constitution/keys/ (who decided each petition), index/ (every catalogue number named is looked up
  for a file), the pool sources the flavor's loader and programs name.
- Writes: out/FLAVOR/ (missing.txt, or the loader, objects, programs and the
  program name).
- Calls: the referee; the compiler (CC, default gcc) with the flags in the script.
- No network, no ports, no registers.

## Size
281 lines in jig/aios (shell).  Needs a C compiler, sed, sort, sha256sum.
