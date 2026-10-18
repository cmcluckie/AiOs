/* PART.share@1: more than one program on one processor.  Ours. */
#ifndef AIOS_SHARE_H
#define AIOS_SHARE_H

#include "kernel.h"
#include "door.h"

/* Run every program in a cpio archive at once.  Never returns: when the last
 * one exits, the machine halts. */
void __attribute__((noreturn)) share(const struct hvm_start_info *si, void *archive,
	unsigned long len, const struct door *door);

/* The running program's heap, or 0 if nothing is being shared. */
struct heap *share_heap(void);

/* The running program is done.  Resumes the next one still running and never
 * returns; returns only if there is none (or nothing is being shared). */
void share_exit(void);

#endif
