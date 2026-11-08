# PART.drag@1 as built

Hand-derived from lifted/PART.drag@1/drag.c on 2026-11-08.  Lifted whole and
untouched from Dave Plummer's software drag race (PlummersSoftwareLLC/Primes),
PrimeC/solution_1/sieve.c and prime-check.h, Mike Koss's C entry of April 2021;
origin in lifted/PART.drag@1/origin.toml.

## What it does
Sieves the odd numbers up to 1,000,000 (--size), skipping multiples of 3 and 5
too (a wheel of 30), one bit a candidate, as many times as it can in five
seconds (--secs) of its own clock, clock(); checks each pass's count against
prime-check.h's table (78,498 below 1,000,000) and says so if it is wrong; then
prints one line, "mckoss-c830;PASSES;SECONDS;1;algorithm=wheel,faithful=yes,
bits=1", and exits 0.  Single-threaded.

## On AiOs
Built by the builder against PART.libc.musl@2 with its author's flags, -Ofast
-march=native -mtune=native -funroll-all-loops (SPEC.builder.3@2), and run by
PART.loader@10 and PART.gate@2 as a Linux program file.  Built on the jig, the
compiler uses the jig's processor: findings 0039 counts 172 SSE and 4 AVX
instructions in it.  It asks for clock_gettime64 about once a pass and for
64 KiB and 4 KiB of memory each pass, given back at its end (findings 0039).

## Every syscall or hardware access
Through musl only: writev, ioctl, brk, mmap2, munmap, clock_gettime64,
set_thread_area, set_tid_address, exit_group (findings 0039 item 3).  No port,
no privileged instruction.

## Size
drag.c 319 lines, prime-check.h 11.
