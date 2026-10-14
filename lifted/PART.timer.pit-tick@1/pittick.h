/* PART.timer.pit-tick@1: the 8254's counter 0, ticking HZ times a second.  See pittick.c for provenance. */
#ifndef AIOS_PITTICK_H
#define AIOS_PITTICK_H

#define PIT_IRQ 0
unsigned int pit_tick_start(void);	/* counter 0 periodic, IRQ 0 every 1/HZ s; returns HZ */

#endif
