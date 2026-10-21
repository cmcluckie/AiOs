# PART.divide64@1 as built

Hand-derived from pool/PART.divide64@1/divide64.c on 2026-10-20.

## What it does
One function, __udivmoddi4(n, d, rem): divides the 64-bit number n by the 64-bit
number d, returns the quotient and, when rem is not null, stores the remainder
there.  The 386 divides 64 bits by 32 in one instruction, but not 64 by 64, so
the compiler calls this helper for that; it normally comes from the compiler's
runtime library (libgcc), which the jig's compiler has none of for 32-bit programs
built without the standard libraries.  musl's printf calls it to print large
numbers.

## How
Long division in base 2, one bit at a time from the top: shift the next bit of n
into the running remainder; if the remainder is at least d, take d away and set
that bit of the quotient.  64 rounds.  It has no divide in it, so the compiler
cannot turn any of it back into a call to itself.

## Edges
Dividing by zero gives a quotient of all ones and leaves n as the remainder;
the real helper traps.  Nothing that calls it in AiOs divides by zero on purpose.

## Everything it touches
No memory but rem, no ports, no registers, no system calls.

## Size
27 lines in divide64.c, 16 of them code.  Ours, in pool/; nothing lifted.
