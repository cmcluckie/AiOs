/* SPDX-License-Identifier: GPL-2.0-only
 *
 * CMP.timer.tsc-pit@1: how fast does the processor's counter tick?  Measured
 * once, against the 8254 timer's 1.193182 MHz, the way Linux does it when
 * nobody will just tell it.  Exposes the counter's rate in kHz, or 0 if the
 * measurement failed.  Requires CAP.dev.pit and the TSC.
 *
 * Lifted from Linux, commit a5ebb76233b79db01e82e061173cb73c6d2b5c6b (2026-10-10).
 * The machine-readable record is origin.toml beside this file; jig/upstream reads it.
 *   arch/x86/kernel/tsc.c                pit_verify_msb, pit_expect_msb 520-546,
 *                                        MAX_QUICK_PIT_* 554-555, quick_pit_calibrate 557-646
 *   arch/x86/include/asm/tsc.h           rdtsc 22-29
 *   arch/x86/include/asm/asm.h           the 32-bit EAX_EDX_* macros 251-253
 *   arch/x86/include/asm/div64.h         do_div 22-25, 30-41
 *   arch/x86/include/asm/shared/io.h     BUILDIO 7-20: the inb and outb instructions
 *   include/linux/i8253.h                PIT_MODE 17, PIT_CH2 19
 *   include/linux/timex.h                PIT_TICK_RATE 160
 *
 * Cut, and why:
 *   has_legacy_pic() check              the machine has no PIC (microvm pic=off), but
 *                                       has the PIT, which is all this needs
 *   the gate on port 0x61               microvm has no port 0x61; QEMU's channel 2 counts
 *                                       with its gate as it is (findings 0008)
 *   both pr_info() lines                no console layer; the caller reports
 *   do_div's power-of-two branch        the divisor here never is one
 *   pit_calibrate_tsc, the slow path,   one way, the fast one; if it fails the caller
 *   and the HPET/PM timer fallbacks     says so rather than trying another
 *   get_cycles(), rdtsc_ordered()       get_cycles() is rdtsc() on x86; called directly
 */

#include "tsccal.h"

/* Glue, ours: what the kernel's headers would have provided.  Linux builds with
 * GNU C and without -Wsign-compare; this build is -std=c11 -Werror. */
typedef unsigned long long u64;
#define asm __asm__
#define get_cycles() rdtsc()
#pragma GCC diagnostic ignored "-Wsign-compare"

/* arch/x86/include/asm/shared/io.h */
static inline unsigned char inb(unsigned short port)
{
	unsigned char v;
	__asm__ volatile("inb %w1, %b0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(unsigned char v, unsigned short port)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(v), "Nd"(port));
}

/* arch/x86/include/asm/asm.h, 32-bit */
#define EAX_EDX_DECLARE_ARGS(val, low, high)	u64 val
#define EAX_EDX_VAL(val, low, high)		(val)
#define EAX_EDX_RET(val, low, high)		"=A" (val)

/* arch/x86/include/asm/tsc.h */
static inline u64 rdtsc(void)
{
	EAX_EDX_DECLARE_ARGS(val, low, high);

	asm volatile("rdtsc" : EAX_EDX_RET(val, low, high));

	return EAX_EDX_VAL(val, low, high);
}

/* arch/x86/include/asm/div64.h, without the power-of-two branch */
#define do_div(n, base)						\
({								\
	unsigned long __upper, __low, __high, __mod, __base;	\
	__base = (base);					\
	{							\
		asm("" : "=a" (__low), "=d" (__high) : "A" (n));\
		__upper = __high;				\
		if (__high) {					\
			__upper = __high % (__base);		\
			__high = __high / (__base);		\
		}						\
		asm("divl %2" : "=a" (__low), "=d" (__mod)	\
			: "rm" (__base), "0" (__low), "1" (__upper));	\
		asm("" : "=A" (n) : "a" (__low), "d" (__high));	\
	}							\
	__mod;							\
})

/* include/linux/i8253.h, include/linux/timex.h */
#define PIT_MODE	0x43
#define PIT_CH2		0x42
#define PIT_TICK_RATE 1193182ul

/* arch/x86/kernel/tsc.c */
static inline int pit_verify_msb(unsigned char val)
{
	/* Ignore LSB */
	inb(0x42);
	return inb(0x42) == val;
}

static inline int pit_expect_msb(unsigned char val, u64 *tscp, unsigned long *deltap)
{
	int count;
	u64 tsc = 0, prev_tsc = 0;

	for (count = 0; count < 50000; count++) {
		if (!pit_verify_msb(val))
			break;
		prev_tsc = tsc;
		tsc = get_cycles();
	}
	*deltap = get_cycles() - prev_tsc;
	*tscp = tsc;

	/*
	 * We require _some_ success, but the quality control
	 * will be based on the error terms on the TSC values.
	 */
	return count > 5;
}

#define MAX_QUICK_PIT_MS 50
#define MAX_QUICK_PIT_ITERATIONS (MAX_QUICK_PIT_MS * PIT_TICK_RATE / 1000 / 256)

static unsigned long quick_pit_calibrate(void)
{
	int i;
	u64 tsc, delta;
	unsigned long d1, d2;
	/*
	 * Counter 2, mode 0 (one-shot), binary count
	 *
	 * NOTE! Mode 2 decrements by two (and then the
	 * output is flipped each time, giving the same
	 * final output frequency as a decrement-by-one),
	 * so mode 0 is much better when looking at the
	 * individual counts.
	 */
	outb(0xb0, 0x43);

	/* Start at 0xffff */
	outb(0xff, 0x42);
	outb(0xff, 0x42);

	/*
	 * The PIT starts counting at the next edge, so we
	 * need to delay for a microsecond. The easiest way
	 * to do that is to just read back the 16-bit counter
	 * once from the PIT.
	 */
	pit_verify_msb(0);

	if (pit_expect_msb(0xff, &tsc, &d1)) {
		for (i = 1; i <= MAX_QUICK_PIT_ITERATIONS; i++) {
			if (!pit_expect_msb(0xff-i, &delta, &d2))
				break;

			delta -= tsc;

			/*
			 * Extrapolate the error and fail fast if the error will
			 * never be below 500 ppm.
			 */
			if (i == 1 &&
			    d1 + d2 >= (delta * MAX_QUICK_PIT_ITERATIONS) >> 11)
				return 0;

			/*
			 * Iterate until the error is less than 500 ppm
			 */
			if (d1+d2 >= delta >> 11)
				continue;

			/*
			 * Check the PIT one more time to verify that
			 * all TSC reads were stable wrt the PIT.
			 *
			 * This also guarantees serialization of the
			 * last cycle read ('d2') in pit_expect_msb.
			 */
			if (!pit_verify_msb(0xfe - i))
				break;
			goto success;
		}
	}
	return 0;

success:
	/*
	 * Ok, if we get here, then we've seen the
	 * MSB of the PIT decrement 'i' times, and the
	 * error has shrunk to less than 500 ppm.
	 *
	 * As a result, we can depend on there not being
	 * any odd delays anywhere, and the TSC reads are
	 * reliable (within the error).
	 *
	 * kHz = ticks / time-in-seconds / 1000;
	 * kHz = (t2 - t1) / (I * 256 / PIT_TICK_RATE) / 1000
	 * kHz = ((t2 - t1) * PIT_TICK_RATE) / (I * 256 * 1000)
	 */
	delta *= PIT_TICK_RATE;
	do_div(delta, i*256*1000);
	return delta;
}

unsigned long tsccal_khz(void)
{
	return quick_pit_calibrate();
}

u64 tsccal_cycles(void)
{
	return rdtsc();
}
