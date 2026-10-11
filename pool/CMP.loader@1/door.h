/* CMP.loader@1: the door.  The only thing a program sees of the machine.
 *
 * The loader calls the program's entry with a pointer to this table.  A program
 * that touches anything else is not using the door, and its as-built spec will
 * say so.  Later petitions append entries; existing offsets never move.
 *
 * Fixed by SPEC.loader.3@1 (plan 6.5).  32-bit cdecl.
 */
#ifndef AIOS_DOOR_H
#define AIOS_DOOR_H

struct door {
	void (*write)(const void *s, unsigned long n);	/* CAP.tty.write: bytes to the console, exactly as given */
	void (*exit)(int code) __attribute__((noreturn));	/* CAP.proc.exit: the program is done; the machine halts */
};

typedef void (*program_entry)(const struct door *);

#endif
