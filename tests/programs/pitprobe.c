/* Probe for findings 0012: why the clock measurement gives up under KVM.
 * Repeats the first 12 steps of Linux's quick_pit_calibrate (lifted in
 * PART.timer.tsc-pit@1) and prints what it saw instead of deciding: for each
 * fall of the 8254's high byte (256 counts, 214.6 us), how many times the
 * byte was read, what the last read cost in counter cycles, and how long the
 * step took.  Then the test Linux applies at step 1.  Runs on the stopwatch's
 * machine, in place of the program.  Not a component; never admitted.
 * Boots on: stopwatch.
 */
#include "door.h"

#define STEPS 12
#define MAX_ITER 233	/* MAX_QUICK_PIT_ITERATIONS: 50 ms of steps */

static const struct door *d;

static void str(const char *s)
{
	unsigned long n = 0;

	while (s[n])
		n++;
	d->write(s, n);
}

static void num(unsigned long v)
{
	char b[11];
	int i = 11;

	do { b[--i] = (char)('0' + v % 10); v /= 10; } while (v);
	d->write(b + i, (unsigned long)(11 - i));
}

static inline unsigned char inb(unsigned short p)
{
	unsigned char v;

	__asm__ volatile("inb %w1, %b0" : "=a"(v) : "Nd"(p));
	return v;
}

static inline void outb(unsigned char v, unsigned short p)
{
	__asm__ volatile("outb %b0, %w1" : : "a"(v), "Nd"(p));
}

static inline unsigned long long rdtsc(void)
{
	unsigned long long v;

	__asm__ volatile("rdtsc" : "=A"(v));
	return v;
}

static int msb_is(unsigned char v)
{
	inb(0x42);
	return inb(0x42) == v;
}

void __attribute__((section(".text.start"), noreturn)) _start(const struct door *door)
{
	unsigned long reads[STEPS], cost[STEPS];
	unsigned long long at[STEPS], t, prev;
	unsigned long delta;
	int i, c;

	d = door;
	outb(0xb0, 0x43);	/* counter 2, mode 0, binary, as Linux */
	outb(0xff, 0x42);
	outb(0xff, 0x42);
	msb_is(0);
	for (i = 0; i < STEPS; i++) {
		t = 0; prev = 0;
		for (c = 0; c < 50000; c++) {
			if (!msb_is((unsigned char)(0xff - i)))
				break;
			prev = t;
			t = rdtsc();
		}
		reads[i] = (unsigned long)c;
		cost[i] = (unsigned long)(rdtsc() - prev);
		at[i] = t;
	}
	str("pitprobe: a step is 256 counts of the 8254, 214.6 us\n");
	for (i = 0; i < STEPS; i++) {
		str("step "); num((unsigned long)i);
		str(": reads "); num(reads[i]);
		str(", last read cost "); num(cost[i]);
		if (i) { str(" cycles, step took "); num((unsigned long)(at[i] - at[i - 1])); }
		str(" cycles\n");
	}
	delta = (unsigned long)(at[1] - at[0]);
	str("linux at step 1: error "); num(cost[0] + cost[1]);
	str(" against a limit of "); num((unsigned long)(((unsigned long long)delta * MAX_ITER) >> 11));
	str(reads[0] > 5 && reads[1] > 5 && cost[0] + cost[1] < (unsigned long)(((unsigned long long)delta * MAX_ITER) >> 11)
		? ": carries on\n" : ": gives up\n");
	d->exit(0);
}
