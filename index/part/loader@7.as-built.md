# PART.loader@7 as built

Hand-derived from the source on 2026-10-13, pending the generator (plan 7.1).
Measured with the gcc in Claude's workspace (not the jig's).

## What it does
Exactly what @6 does (index/part/loader@6.as-built.md), built with
PART.fs.minix@2 instead of @1: the save system call now writes over a file
that is already there.  The system call table is @6's, unchanged; nothing is
appended.

## Every syscall or hardware access
As @6.

## Every symbol it imports
As @6, with minix_load and minix_save from PART.fs.minix@2.

## Every byte it writes anywhere
As @6.

## Size
loader.c and door.h differ from @6 in comments, one #include and nothing else;
19704 bytes as built.
