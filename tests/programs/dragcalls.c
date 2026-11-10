/* Probe for TST.gate.clock@1, TST.gate.munmap@1 and TST.gate.avx@1: a Linux
 * program file with no C library, asking for what Dave Plummer's sieve asks for
 * (findings 0039), one thing per build, chosen with -DCLOCK, -DMUNMAP or -DAVX.
 * Built static for 32-bit x86 at Linux's usual address.  Not a component; never
 * admitted.
 *   CLOCK   clock_gettime64(CLOCK_PROCESS_CPUTIME_ID), the call behind the
 *           sieve's clock(): its milliseconds, one line each time they pass the
 *           next half second, as tests/programs/msprobe.c prints the ms door's;
 *           "clock_gettime64 N" and stop if the call fails
 *   MUNMAP  64 KiB asked for with mmap2, written, and given back with munmap,
 *           20,000 times (1.25 GiB in all, on a 64 MiB machine), as each sieve
 *           pass does; "munmap 20000 rounds", or where mmap2 failed
 *   KEEP    two 64 KiB blocks taken, the first given back while the second is
 *           still in use, then two more taken: neither may land on the
 *           second, and what was written there must still be there; "keep ok",
 *           or what went wrong
 *   AVX     one SSE and one AVX instruction, each checked: "sse ok", "avx ok";
 *           SSE is on already (TST.gate.sse@1), AVX is not */
static long sys(long n, long a, long b, long c, long d, long e)
{
	long r;
	__asm__ volatile("pushl %%ebp; movl %7, %%ebp; int $0x80; popl %%ebp"
		: "=a"(r) : "0"(n), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e), "g"(0L) : "memory");
	return r;
}
static void say(const char *s, long v, int with)
{
	char b[64];
	int n = 0, k = 0;
	unsigned long u = v < 0 ? (unsigned long)-v : (unsigned long)v;
	char d[12];

	while (*s)
		b[n++] = *s++;
	if (with) {
		if (n)
			b[n++] = ' ';
		if (v < 0)
			b[n++] = '-';
		do { d[k++] = (char)('0' + u % 10); u /= 10; } while (u);
		while (k)
			b[n++] = d[--k];
	}
	b[n++] = '\n';
	sys(4, 1, (long)b, n, 0, 0);
}
#define NR_EXIT_GROUP		252
#define NR_MMAP2		192
#define NR_MUNMAP		91
#define NR_CLOCK_GETTIME64	403
#define CLOCK_PROCESS_CPUTIME_ID 2
void _start(void)
{
#if defined(CLOCK)
	struct { long long sec; long long nsec; } ts;
	unsigned long next = 0, ms;
	long r;

	for (;;) {
		r = sys(NR_CLOCK_GETTIME64, CLOCK_PROCESS_CPUTIME_ID, (long)&ts, 0, 0, 0);
		if (r) {
			say("clock_gettime64", r, 1);
			break;
		}
		ms = (unsigned long)ts.sec * 1000 + (unsigned long)ts.nsec / 1000000;	/* 32-bit: no divide helper (findings 0032) */
		if (ms >= next) {
			say("", (long)ms, 1);
			next += 500;
		}
	}
#elif defined(MUNMAP)
	long i, p, r;

	for (i = 0; i < 20000; i++) {
		p = sys(NR_MMAP2, 0, 65536, 3, 0x22, -1);	/* PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS */
		if (p < 0 && p > -4096) {
			say("mmap2 failed at round", i, 1);
			say("mmap2", p, 1);
			sys(NR_EXIT_GROUP, 1, 0, 0, 0, 0);
		}
		((volatile unsigned char *)p)[65535] = 1;
		r = sys(NR_MUNMAP, p, 65536, 0, 0, 0);
		if (r) {
			say("munmap", r, 1);
			sys(NR_EXIT_GROUP, 1, 0, 0, 0, 0);
		}
	}
	say("munmap rounds", i, 1);
#elif defined(KEEP)
	long a, b, c, d;
	volatile unsigned char *kept;

	a = sys(NR_MMAP2, 0, 65536, 3, 0x22, -1);
	b = sys(NR_MMAP2, 0, 65536, 3, 0x22, -1);
	if ((a < 0 && a > -4096) || (b < 0 && b > -4096)) {
		say("mmap2 failed", a < 0 && a > -4096 ? a : b, 1);
		sys(NR_EXIT_GROUP, 1, 0, 0, 0, 0);
	}
	kept = (volatile unsigned char *)b;
	kept[0] = 0x5a;
	kept[65535] = 0x5a;
	if (sys(NR_MUNMAP, a, 65536, 0, 0, 0)) {
		say("munmap of the first block failed", 0, 0);
		sys(NR_EXIT_GROUP, 1, 0, 0, 0, 0);
	}
	c = sys(NR_MMAP2, 0, 65536, 3, 0x22, -1);
	d = sys(NR_MMAP2, 0, 65536, 3, 0x22, -1);
	if ((c < 0 && c > -4096) || (d < 0 && d > -4096)) {
		say("mmap2 after munmap failed", c < 0 && c > -4096 ? c : d, 1);
		sys(NR_EXIT_GROUP, 1, 0, 0, 0, 0);
	}
	if ((unsigned long)c - (unsigned long)b < 65536 || (unsigned long)b - (unsigned long)c < 65536 ||
	    (unsigned long)d - (unsigned long)b < 65536 || (unsigned long)b - (unsigned long)d < 65536) {
		say("a block still in use was handed out again", 0, 0);
		sys(NR_EXIT_GROUP, 1, 0, 0, 0, 0);
	}
	((volatile unsigned char *)c)[0] = 0xa5;
	((volatile unsigned char *)d)[0] = 0xa5;
	say(kept[0] == 0x5a && kept[65535] == 0x5a ? "keep ok" : "keep overwritten", 0, 0);
#elif defined(AVX)
	static unsigned int a[8] __attribute__((aligned(32))) = { 1, 2, 3, 4, 5, 6, 7, 8 };
	static unsigned int b[8] __attribute__((aligned(32))) = { 10, 20, 30, 40, 50, 60, 70, 80 };
	static unsigned int c[8] __attribute__((aligned(32)));

	__asm__ volatile("movdqa %1, %%xmm0; paddd %2, %%xmm0; movdqa %%xmm0, %0"
		: "=m"(*(unsigned int (*)[4])c) : "m"(*(unsigned int (*)[4])a), "m"(*(unsigned int (*)[4])b) : "xmm0", "memory");
	say(c[0] == 11 && c[3] == 44 ? "sse ok" : "sse wrong", 0, 0);
	__asm__ volatile("vmovdqa %1, %%ymm1; vaddps %2, %%ymm1, %%ymm1; vmovdqa %%ymm1, %0"
		: "=m"(*(unsigned int (*)[8])c) : "m"(*(unsigned int (*)[8])a), "m"(*(unsigned int (*)[8])b) : "xmm1", "memory");
	say(c[7] != 0 ? "avx ok" : "avx wrong", 0, 0);
#endif
	sys(NR_EXIT_GROUP, 0, 0, 0, 0, 0);
	for (;;)
		;
}
