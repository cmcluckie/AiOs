/* CMP.loader@4: the door.  The only thing a program sees of the machine.
 *
 * The loader calls the program's entry with a pointer to this table.  A program
 * that touches anything else is not using the door, and its as-built spec will
 * say so.  Later petitions append entries; existing offsets never move.
 *
 * Fixed by SPEC.loader.3@1 (plan 6.5).  32-bit cdecl.  @2 appended alloc at offset 8; @3 read at 12; @4 appends ms at 16.
 */
#ifndef AIOS_DOOR_H
#define AIOS_DOOR_H

struct door {
	void (*write)(const void *s, unsigned long n);	/* CAP.tty.write: bytes to the console, exactly as given */
	void (*exit)(int code) __attribute__((noreturn));	/* CAP.proc.exit: the program is done; the machine halts */
	void *(*alloc)(unsigned long bytes);			/* CAP.mem.alloc: bytes of RAM, 16-aligned, or 0 if the machine has no more; never freed */
	unsigned long (*read)(void *buf, unsigned long n);	/* CAP.tty.read: waits for the first byte, returns it and any already waiting, up to n */
	unsigned long (*ms)(void);				/* CAP.time.read: milliseconds since the loader started the clock; 0 forever if it could not */
};

typedef void (*program_entry)(const struct door *);

#endif
