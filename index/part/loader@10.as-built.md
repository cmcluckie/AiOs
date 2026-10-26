# PART.loader@10 as built

Hand-derived from the source on 2026-10-20, pending the generator (plan 7.1).
Measured with the gcc and QEMU in Claude's workspace (software emulation), not the jig's.

## What it does
Everything PART.loader@9 does (its as-built spec still holds), and one more kind of
program (part 20, findings 0023 item 10): when the program the machine hands over
starts with ELF's magic bytes, it is a Linux program file.  The loader refuses it,
with a line "loader: ..." on the console and a halt, unless it is 32-bit, x86,
little-endian and an executable that runs where it is linked (ET_EXEC), its program
headers are where the file says, no piece asks for a dynamic linker (PT_INTERP),
every piece lies in the first 8 MiB of its window, and the machine has RAM for the
window and did not put the file inside it.  Then it zeroes the window (16 MiB of
physical RAM from 4 MiB), copies each loadable piece to where its address says, and
lays out the stack at the top the way Linux's create_elf_tables does: the 16
AT_RANDOM bytes, the argument strings, then (16-byte aligned) argc, the argument
pointers, a zero, no environment, a zero, and the facts AT_PHDR, AT_PHENT, AT_PHNUM,
AT_PAGESZ (4,096), AT_ENTRY, AT_RANDOM and AT_NULL.  The arguments are the kernel
command line split on spaces, at most 15.  Then PART.gate@2's gate_exec runs it,
its heap starting at the page after its last piece.

The AT_RANDOM bytes come from rdrand where CPUID says the processor has it
(leaf 1, ECX bit 30), four words, up to ten tries each, as Linux does (decision
0078, part 21).  TECH-DEBT 4: on a processor without rdrand, or when a word fails
ten tries, that word is the cycle counter, not random (decision 0046, findings 0021).

## Every syscall or hardware access
As @9, and for AT_RANDOM cpuid once, then rdrand (rdtsc only on the fallback).

## Every symbol it imports
As @9, and gate_exec (PART.gate@2).

## Every byte it writes anywhere
As @9, and for a Linux program file: the 16 MiB window (zeroed, then the file's
pieces), the stack table and strings at its top, and a refusal line on the console.

## Size
loader.c 611 lines, 430 of them code (@9: 303).  The kernel file for the cat flavor:
24,852 bytes (text 13,916, data 124, bss 149,704) since decision 0065 (part 21),
when the Minix write code stopped being built into a kernel whose petition only
reads files; 29,048 bytes (text 16,524, bss 153,800) before.
