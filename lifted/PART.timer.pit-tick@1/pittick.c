/* SPDX-License-Identifier: GPL-2.0-only
 *
 * PART.timer.pit-tick@1: the 8254's counter 0 in periodic mode, raising IRQ 0
 * HZ times a second: the tick a scheduler takes turns on.  HZ is 100, Linux's
 * user-visible default since the beginning.  Requires CAP.dev.pit.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   drivers/clocksource/i8253.c        pit_set_periodic, its three writes 158-161
 *   include/linux/i8253.h              PIT_MODE 17, PIT_CH0 18, PIT_LATCH 21
 *   include/linux/timex.h              PIT_TICK_RATE 160
 *   include/uapi/asm-generic/param.h   __USER_HZ and HZ 5-11
 *   arch/x86/include/asm/shared/io.h   BUILDIO 7-20: the outb instruction
 *
 * Cut, and why:
 *   i8253_lock                         one processor, interrupts off at start
 *   outb_p's slow-down                 no clock to slow down by (findings 0011)
 *   the clockevent framework           one tick, one handler, nothing to choose
 *   oneshot mode, shutdown             periodic only, never stopped
 */

#include "pittick.h"

/* arch/x86/include/asm/shared/io.h */
static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(v), "Nd"(port));
}
#define outb_p outb

/* include/uapi/asm-generic/param.h */
#ifndef __USER_HZ
#define __USER_HZ	100
#endif

#ifndef HZ
#define HZ __USER_HZ
#endif

/* include/linux/timex.h */
#define PIT_TICK_RATE 1193182ul

/* include/linux/i8253.h */
#define PIT_MODE	0x43
#define PIT_CH0		0x40
#define PIT_LATCH	((PIT_TICK_RATE + HZ/2) / HZ)

unsigned int pit_tick_start(void)
{
	/* i8253.c pit_set_periodic */
	/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(0x34, PIT_MODE);
	outb_p(PIT_LATCH & 0xff, PIT_CH0);	/* LSB */
	outb_p(PIT_LATCH >> 8, PIT_CH0);	/* MSB */
	return HZ;
}
