/* PART.timer.kvmclock@1: the processor counter's rate, as KVM reports it.  See kvmclock.c for provenance. */
#ifndef AIOS_KVMCLOCK_H
#define AIOS_KVMCLOCK_H

unsigned long kvmclock_khz(void);	/* counter ticks per millisecond, or 0 if there is no KVM to ask */

#endif
