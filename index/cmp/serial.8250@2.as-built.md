# CMP.serial.8250@2 as built

Hand-derived from the source and the disassembly on 2026-10-11, pending the
generator (plan 7.1).  Built with the jig's gcc 13.3, `-m32 -O2 -ffreestanding`.

## What it does
Initialises a 16550 at a given I/O base to 8n1, no interrupts, no FIFO, DTR and
RTS asserted, divisor 1 (115200 baud from the PC reference clock); writes bytes one
at a time after waiting for the transmit holding register; waits for the transmitter
to drain.

@2 adds: reports whether a received byte is waiting (LSR bit 0), and takes
one from the receive register, waiting if none is.

## Every syscall or hardware access
I/O ports `base+0` (TX, DLL), `base+1` (IER, DLM), `base+2` (FCR), `base+3` (LCR),
`base+4` (MCR), `base+5` (LSR); @2 also reads `base+0` (RX).  Ten `out`, four `in` instructions across the whole
loader image, all in this segment.  Nothing else.

## Every symbol it imports
None.

## Every byte it writes anywhere
The bytes it is handed, to `base+0`, unchanged.  Nine configuration bytes to the
registers above during init.

## Size
7 functions.  Source: 69 lines of code (57 from @1 plus 12), lifted; provenance
in the file header.  Code size not yet re-measured with the jig's compiler; the
240 bytes recorded for @1 no longer apply (another gcc puts @2 at 88 bytes more
than @1).

## What was cut
Listed in serial8250.c and origin.toml.  The init sequence is kept though the hypervisor does not
need it (findings 0002 #7).
