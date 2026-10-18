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

#endif
