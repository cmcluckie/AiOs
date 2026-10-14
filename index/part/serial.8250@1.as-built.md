# PART.serial.8250@1 as built

Hand-derived from the source and the disassembly on 2026-10-10, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`.

## What it does
Initialises a 16550 at a given I/O base to 8n1, no interrupts, no FIFO, DTR and
RTS asserted, divisor 1 (115200 baud from the PC reference clock); writes bytes one
at a time after waiting for the transmit holding register; waits for the transmitter
to drain.

## Every syscall or hardware access
I/O ports `base+0` (TX, DLL), `base+1` (IER, DLM), `base+2` (FCR), `base+3` (LCR),
`base+4` (MCR), `base+5` (LSR).  Ten `out`, four `in` instructions across the whole
loader image, all in this part.  Nothing else.

## Every symbol it imports
None.

## Every byte it writes anywhere
The bytes it is handed, to `base+0`, unchanged.  Nine configuration bytes to the
registers above during init.

## Size
5 functions, 240 bytes of code.  Source: 57 lines of code, lifted; provenance in the file header.

## What was cut
Listed in serial8250.c and origin.toml.  The init sequence is kept though the hypervisor does not
need it (findings 0002 #7).
