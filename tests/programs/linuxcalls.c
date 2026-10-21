/* Probe for TST.gate.linux-refuses@1: a Linux program file with no C library,
 * asking each of PART.gate@2's Linux calls for something it does not do, and
 * printing each answer as "name answer", one per line.  The answers expected
 * are Linux's own error numbers, negated, as the call returns them.  Built
 * static for 32-bit x86 at Linux's usual address.  Not a component; never
 * admitted. */
static long sys(long n, long a, long b, long c, long d, long e)
{
	long r;
	__asm__ volatile("pushl %%ebp; movl %7, %%ebp; int $0x80; popl %%ebp"
		: "=a"(r) : "0"(n), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e), "g"(0L) : "memory");
	return r;
}
static void out(const char *name, long v)
{
	char b[48];
	int n = 0, k;
	unsigned long u = v < 0 ? (unsigned long)-v : (unsigned long)v;
	char d[12];

	while (*name)
		b[n++] = *name++;
	b[n++] = ' ';
	if (v < 0)
		b[n++] = '-';
	k = 0;
	do { d[k++] = (char)('0' + u % 10); u /= 10; } while (u);
	while (k)
		b[n++] = d[--k];
	b[n++] = '\n';
	sys(4, 1, (long)b, n, 0, 0);
}
static unsigned long desc[4];
void _start(void)
{
	long m, brk0, i, bad = 0;
	unsigned char *p;
	struct { unsigned long base, len; } iov = { 0, 1 };

	out("open-for-writing", sys(5, (long)"/one", 1, 0, 0, 0));
	out("open-below-root", sys(5, (long)"/a/b", 0, 0, 0, 0));
	out("open-missing", sys(5, (long)"/nosuch", 0, 0, 0, 0));
	out("read-not-open", sys(3, 9, (long)desc, 1, 0, 0));
	iov.base = (unsigned long)desc;
	out("writev-not-open", sys(146, 5, (long)&iov, 1, 0, 0));
	out("write-kernel-buffer", sys(4, 1, 0x100000, 16, 0, 0));
	out("mmap2-shared", sys(192, 0, 4096, 3, 0x01, -1));
	m = sys(192, 0, 8192, 3, 0x22, -1);
	p = (unsigned char *)m;
	for (i = 0; m > 0 && i < 8192; i++)
		bad |= p[i];
	out("mmap2-fresh-zero", m > 0x08000000L && m < 0x09000000L && !bad ? 0 : -1);
	desc[0] = 3; desc[1] = 0; desc[2] = 0xfffff; desc[3] = 0x51;
	out("set_thread_area-entry-3", sys(243, (long)desc, 0, 0, 0, 0));
	out("statx-by-name", sys(383, -100, (long)"one", 0, 0x7ff, (long)desc));
	brk0 = sys(45, 0, 0, 0, 0, 0);
	out("brk-past-window", sys(45, 0x7ffff000L, 0, 0, 0, 0) == brk0 ? 0 : -1);
	out("fork", sys(2, 0, 0, 0, 0, 0));
	sys(252, 0, 0, 0, 0, 0);
	for (;;)
		;
}
