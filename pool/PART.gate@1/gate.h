/* PART.gate@1: programs run in ring 3 and reach the kernel only by int 0x80.  Ours. */
#ifndef AIOS_GATE_H
#define AIOS_GATE_H

#include "kernel.h"
#include "door.h"

/*
 * Take the machine over from the loader's first moments and give it to the
 * program: new descriptor table with ring 3 segments, the task state segment,
 * the interrupt table, page tables that close everything to the program but
 * its own window, and then ring 3, at ENTRY.  The program is handed the system
 * call table that is built into this part (stubs that run int 0x80), never the
 * kernel's own.  Its window is [LO, HI): every address it may touch besides its
 * stack and that table.  KERNEL_CALLS is what the kernel does for each call.
 * Never returns: the program exits through the exit call, or is stopped.
 */
void __attribute__((noreturn)) gate_run(unsigned long entry, unsigned long lo, unsigned long hi,
	const struct door *kernel_calls);


/*
 * Several programs (PART.share@2).  A space is one program's view of the
 * machine: the same virtual window in every space, mapped to its own physical
 * RAM, and a kernel stack of its own.  All of this is done before the gate
 * starts, with paging off, so PHYS is an address the kernel can write.
 */
#define GATE_MAX_SPACES 4

struct gate_space;

/* The sharing part's two decisions.  Each is told which space was running and
 * returns the one to run next (the same one to carry on).  irq: a hardware
 * interrupt arrived; fault: the processor stopped the program (the vector and the
 * address that caused it); it must not return the stopped space. */
struct gate_sched {
	struct gate_space *(*irq)(unsigned long vec, struct gate_space *cur);
	struct gate_space *(*fault)(unsigned long vec, unsigned long addr, struct gate_space *cur);
};

/* Register the scheduler, and what the kernel does for each call.  Before gate_start. */
void gate_schedule(const struct gate_sched *s);
void gate_share_calls(const struct door *kernel_calls);

/* A space whose window [VIRT, VIRT + LEN) is the physical RAM from PHYS, entered
 * at ENTRY with its stack growing down from STACK_TOP (inside the window).  Null
 * if it does not fit or the window differs from the first space's. */
struct gate_space *gate_space_new(unsigned long phys, unsigned long virt, unsigned long len,
	unsigned long entry, unsigned long stack_top);

/* Take the machine over (tables, paging) and run FIRST in ring 3.  Never returns. */
void __attribute__((noreturn)) gate_start(struct gate_space *first);

/* Run that space now, from its saved registers.  Never returns. */
void __attribute__((noreturn)) gate_resume(struct gate_space *s);

#endif
