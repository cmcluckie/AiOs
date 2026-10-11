/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.serial.8250@1: the lines that put a byte on a 16550, and nothing holding
 * their hand.  Exposes CAP.tty.write.  Requires CAP.dev.serial.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10):
 *   drivers/tty/serial/8250/8250_early.c   serial_putc 86-98, init_port 131-152
 *   arch/x86/kernel/early_printk.c         early_serial_putc 108-116, early_serial_hw_init 128-141
 *   arch/x86/include/asm/shared/io.h       BUILDIO 7-20: the inb/outb instructions
 *   include/uapi/linux/serial_reg.h        the register offsets and bits named below
 *   include/linux/serial.h                 uart_lsr_tx_empty 26-29
 *
 * Cut, and why:
 *   struct uart_port, struct earlycon_device, struct console   one port, passed as a number
 *   the iotype switch (MMIO, MMIO16, MMIO32, MMIO32BE)        the machine's port is I/O-mapped
 *   regshift                                                   16550 registers are byte-spaced
 *   uart_console_write's '\n' -> '\r\n' translation            SPEC.hello.1 is byte-exact
 *   the 0xffff timeout in early_serial_putc                    a port that never drains is a TST
 *                                                              timeout, not a silent byte drop
 *   cpu_relax                                                  one CPU, nothing to yield to
 *   EARLYCON_DECLARE, OF_EARLYCON_DECLARE, console_poll read   the console and tty layers
 *   static_call indirection                                    one implementation
 *
 * Kept although the hypervisor does not need it: serial8250_init.  QEMU's 16550
 * transmits whatever the line control and divisor say, so no test on the jig can
 * turn these nine writes red.  A real 16550 needs them.  Findings 0002 #7.
 */

#include "serial8250.h"

/* arch/x86/include/asm/shared/io.h */
static inline unsigned char inb(unsigned short port)
{
	unsigned char v;
	__asm__ volatile("inb %w1, %b0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(v), "Nd"(port));
}

/* include/uapi/linux/serial_reg.h */
#define UART_TX		0	/* Out: Transmit buffer */
#define UART_IER	1	/* Out: Interrupt Enable Register */
#define UART_FCR	2	/* Out: FIFO Control Register */
#define UART_LCR	3	/* Out: Line Control Register */
#define UART_MCR	4	/* Out: Modem Control Register */
#define UART_LSR	5	/* In:  Line Status Register */
#define UART_DLL	0	/* Out: Divisor Latch Low */
#define UART_DLM	1	/* Out: Divisor Latch High */
#define UART_LCR_DLAB	0x80	/* Divisor latch access bit */
#define UART_LCR_WLEN8	0x03	/* Wordlength: 8 bits */
#define UART_MCR_RTS	0x02
#define UART_MCR_DTR	0x01
#define UART_LSR_TEMT	0x40	/* Transmitter empty */
#define UART_LSR_THRE	0x20	/* Transmit-hold-register empty */

/* 115200 baud from the 1.8432 MHz reference clock a PC 16550 has: 1843200 / (16 * 115200). */
#define DIVISOR		1

/* 8250_early.c init_port; early_printk.c early_serial_hw_init */
void serial8250_init(unsigned short base)
{
	unsigned char lcr;

	outb(UART_LCR_WLEN8, base + UART_LCR);			/* 8n1 */
	outb(0, base + UART_IER);				/* no interrupt */
	outb(0, base + UART_FCR);				/* no fifo */
	outb(UART_MCR_DTR | UART_MCR_RTS, base + UART_MCR);

	lcr = inb(base + UART_LCR);
	outb(lcr | UART_LCR_DLAB, base + UART_LCR);
	outb(DIVISOR & 0xff, base + UART_DLL);
	outb((DIVISOR >> 8) & 0xff, base + UART_DLM);
	outb(lcr & ~UART_LCR_DLAB, base + UART_LCR);
}

/* early_printk.c early_serial_putc: wait for the holding register, then write */
void serial8250_putc(unsigned short base, unsigned char c)
{
	while ((inb(base + UART_LSR) & UART_LSR_THRE) == 0)
		;
	outb(c, base + UART_TX);
}

void serial8250_write(unsigned short base, const void *s, unsigned long n)
{
	const unsigned char *p = s;

	while (n--)
		serial8250_putc(base, *p++);
}

/* serial.h uart_lsr_tx_empty: both the holding register and the shift register empty */
void serial8250_flush(unsigned short base)
{
	while ((inb(base + UART_LSR) & (UART_LSR_TEMT | UART_LSR_THRE))
	       != (UART_LSR_TEMT | UART_LSR_THRE))
		;
}
