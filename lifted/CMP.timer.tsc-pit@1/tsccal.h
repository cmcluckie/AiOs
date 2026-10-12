/* CMP.timer.tsc-pit@1: the processor counter's rate, measured against the 8254.  See tsccal.c for provenance. */
#ifndef AIOS_TSCCAL_H
#define AIOS_TSCCAL_H

unsigned long tsccal_khz(void);			/* counter ticks per millisecond, or 0 if the measurement failed */
unsigned long long tsccal_cycles(void);	/* the counter, now */

#endif
