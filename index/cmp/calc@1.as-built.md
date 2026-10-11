# CMP.calc@1 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`.

## What it does
Writes a one-line banner and a prompt, then reads one byte at a time through
the read door.  Printable bytes go into an 80-byte line and are echoed;
backspace removes one and echoes "\b \b"; Enter (\r, \n, or \r\n as one)
evaluates the line as reverse-Polish tokens against a 64-deep stack of 32-bit
integers and writes "= top", "(empty)" or one error line.  q, quit, Ctrl-C and
Ctrl-D write "bye" and call the exit door.

## Every syscall or hardware access
None.  Indirect calls through the door table only: read, write, exit.  No
`in`, `out`, no privileged instruction.  Division is guarded so that neither
divide-by-zero nor INT_MIN / -1 can raise the processor's divide fault, which
on this machine (no IDT) would end it.

## Every symbol it imports
None.

## Every byte it writes anywhere
To the write door: the banner, prompts, echoed input, results and errors.  To
memory: its 256-byte stack array and depth, carried in the file as zeros, an
80-byte line buffer and small frames on the loader's stack.

## Size
2864 bytes.
