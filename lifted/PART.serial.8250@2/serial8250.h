/* PART.serial.8250@2: the lines that put a byte on a 16550, and take one off.  See serial8250.c for provenance. */
#ifndef AIOS_SERIAL8250_H
#define AIOS_SERIAL8250_H

void serial8250_init(unsigned short base);
void serial8250_putc(unsigned short base, unsigned char c);
void serial8250_write(unsigned short base, const void *s, unsigned long n);
void serial8250_flush(unsigned short base);
int serial8250_poll(unsigned short base);
unsigned char serial8250_getc(unsigned short base);

#endif
