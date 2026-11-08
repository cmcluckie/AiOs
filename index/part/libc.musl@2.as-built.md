# PART.libc.musl@2 as built

PART.libc.musl@1 (index/part/libc.musl@1.as-built.md) with 29 source files more,
and the 8 headers they need, all whole and untouched from musl 1.2.4 at the pin
in jig/musl.pin; origin in lifted/PART.libc.musl@2/origin.toml.  Found with
jig/libc pieces on Dave Plummer's sieve and PART.divide64@1: 84 pieces, 29 of
them not in @1 (findings 0042).

## What the 29 add
printf, puts, sscanf and the number scanning under them (vfscanf, floatscan,
intscan, shgetc, mbrtowc, mbsinit); calloc and realloc; assert and abort, with
raise and the signal blocking they use; strcmp; clock and clock_gettime, with
musl's vDSO lookup (vdso.c), which on AiOs finds none and makes the system call;
mremap; and the long-double helpers printf's floating point takes (copysignl,
fabsl, fmodl, scalbn, scalbnl).

## On AiOs
Built by the builder the way @1 is (jig/aios, libc_musl), for every program whose
catalogue entry depends on PART.libc.musl@2.  Cat keeps @1.

## Size
114 source files (111 under src/, 3 start files) and their headers: 216 files.
