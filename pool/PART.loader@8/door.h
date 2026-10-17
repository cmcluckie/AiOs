/* PART.loader@8: the system call table, unchanged since @5 (called the door
 * until findings 0014; the file and the struct keep the old name).  The only
 * thing a program sees of the machine.
 *
 * The kernel calls the program's entry with a pointer to this table.  A program
 * that touches anything else is not using it, and its as-built spec will say
 * so.  Later petitions append entries; existing offsets never move.
 *
 * Fixed by SPEC.loader.3@1 (plan 6.5).  32-bit cdecl.  @2 appended alloc at
 * offset 8; @3 read at 12; @4 ms at 16; @5 load at 20 and save at 24.  @6, @7
 * and @8 append nothing.  From @8 every entry is always there, and an entry
 * whose part the petition did not name answers the way it does when the
 * hardware is missing: ms says 0, load and save say -1.
 */
#ifndef AIOS_DOOR_H
#define AIOS_DOOR_H

struct door {
	void (*write)(const void *s, unsigned long n);	/* CAP.tty.write: bytes to every console, exactly as given */
	void (*exit)(int code) __attribute__((noreturn));	/* CAP.proc.exit: the program is done; the machine halts */
	void *(*alloc)(unsigned long bytes);			/* CAP.mem.alloc: bytes of RAM, 16-aligned, or 0 if the machine has no more; never freed */
	unsigned long (*read)(void *buf, unsigned long n);	/* CAP.tty.read: waits for the first byte, returns it and any already waiting, up to n */
	unsigned long (*ms)(void);				/* CAP.time.read: milliseconds since the kernel started the clock; 0 forever if it could not */
	long (*load)(const char *name, void *buf, unsigned long max);	/* CAP.fs.read: a file from the disk's root directory, up to max bytes; its length, or -1 */
	long (*save)(const char *name, const void *buf, unsigned long n);	/* CAP.fs.write: a file in the disk's root directory, new or written over; 0, or -1 */
};

typedef void (*program_entry)(const struct door *);

#endif
