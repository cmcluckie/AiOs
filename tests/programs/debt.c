/* Probe for TST.gate.linux-only@1, TST.loader.random@1 and TST.gate.sse@1: a
 * Linux program file with no C library, booted under the cat flavor's kernel
 * (PART.loader@10, PART.gate@2).  Its first argument says what to show:
 *   numbers  what the kernel answers at 1000 to 1003, the AiOs-only numbers of
 *            TECH-DEBT 1; Linux has no call there, so each should be -38 (ENOSYS)
 *   random   the 16 bytes AT_RANDOM points at, in hex (TECH-DEBT 4)
 *   sse      four sums done with SSE instructions (findings 0023 item 13);
 *            this file is built with -msse2 for that mode's binary
 * Each answer is a line, then exit 0.  Not a component; never admitted. */
static long sys(long n, long a, long b, long c)
{
	long r;
	__asm__ volatile("int $0x80" : "=a"(r) : "0"(n), "b"(a), "c"(b), "d"(c) : "memory");
	return r;
}
static void say(const char *s, int n) { sys(4, 1, (long)s, n); }
static void num(const char *name, long v)
{
	char b[48], d[12];
	int n = 0, k = 0;
	unsigned long u = v < 0 ? (unsigned long)-v : (unsigned long)v;

	while (*name)
		b[n++] = *name++;
	b[n++] = ' ';
	if (v < 0)
		b[n++] = '-';
	do { d[k++] = (char)('0' + u % 10); u /= 10; } while (u);
	while (k)
		b[n++] = d[--k];
	b[n++] = '\n';
	say(b, n);
}
static int same(const char *a, const char *b)
{
	while (*a && *a == *b)
		a++, b++;
	return *a == *b;
}
#ifdef SSE
typedef float v4 __attribute__((vector_size(16)));
static volatile float in[8] = { 1, 2, 3, 4, 10, 20, 30, 40 };
#endif
void __attribute__((used)) start_c(unsigned long *sp)
{
	long argc = (long)sp[0];
	const char *what = argc > 1 ? (const char *)sp[2] : "";
	unsigned long *p = sp + argc + 2;	/* past argv and its zero */

	while (*p)				/* past the environment and its zero */
		p++;
	p++;
	if (same(what, "numbers")) {
		num("nr-1000", sys(1000, 0, 0, 0));
		num("nr-1001", sys(1001, 0, 0, 0));
		num("nr-1002", sys(1002, 0, 0, 0));
		num("nr-1003", sys(1003, 0, 0, 0));
	} else if (same(what, "random")) {
		const unsigned char *r = 0;
		char h[33];
		int i;

		for (; p[0]; p += 2)
			if (p[0] == 25)
				r = (const unsigned char *)p[1];
		if (!r) {
			say("no AT_RANDOM\n", 13);
		} else {
			for (i = 0; i < 16; i++) {
				h[2 * i] = "0123456789abcdef"[r[i] >> 4];
				h[2 * i + 1] = "0123456789abcdef"[r[i] & 15];
			}
			h[32] = '\n';
			say(h, 33);
		}
#ifdef SSE
	} else if (same(what, "sse")) {
		v4 a = { in[0], in[1], in[2], in[3] }, b = { in[4], in[5], in[6], in[7] }, c = a + b;
		num("sum-0", (long)c[0]); num("sum-1", (long)c[1]); num("sum-2", (long)c[2]); num("sum-3", (long)c[3]);
#endif
	} else {
		say("what?\n", 6);
	}
	sys(252, 0, 0, 0);
	for (;;)
		;
}
__asm__(".globl _start\n_start:\n\tmovl %esp, %eax\n\tandl $-16, %esp\n\tpushl %eax\n\tpushl %eax\n\tcall start_c\n");
